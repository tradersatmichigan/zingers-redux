#ifndef EXCHANGE_HPP
#define EXCHANGE_HPP

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Asset.hpp"

enum Side : uint8_t {
  BUY = 0,
  SELL = 1,
};

struct Trade {
  int buyer_id;
  int seller_id;
  int price;
  int volume;
  int order_id;
};

struct Order {
  Side side;
  int user_id;
  int price;
  int volume;
  int order_id;

  Order(Side side, int user_id, int price, int volume, int order_id)
      : side(side),
        user_id(user_id),
        price(price),
        volume(volume),
        order_id(order_id){};
};

struct OrderResult {
  std::optional<std::string_view> error;
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
};

struct CancelResult {
  std::optional<std::string_view> error;
  std::optional<int> order_id;
};

struct Cash {
  int amount_held;
  int buying_power;
};

struct AssetAmount {
  int amount_held;
  int selling_power;
};

struct Exchange {
  /* Used by all instances of Exchange */
  static inline std::unordered_map<int, Cash> user_cash;
  static inline std::mutex cash_mutex;

  /* Per-exchange information */
  Asset asset;
  std::unordered_map<int, AssetAmount> user_assets;
  std::map<int, std::deque<Order>, std::greater<>> buy_orders;
  std::map<int, std::deque<Order>> sell_orders;
  std::unordered_map<int, std::deque<Order>::iterator> all_orders;
  int order_number{0};

  Exchange(Asset asset) : asset(asset) {}

  auto register_user(int user_id, int cash, int assets) -> void {
    std::scoped_lock lock(cash_mutex);
    if (user_assets.contains(user_id)) {
      return;
    }
    if (!user_cash.contains(user_id)) {
      user_cash[user_id] = {cash, cash};
    }
    user_assets[user_id] = {assets, assets};
  }

  [[nodiscard]] auto validate_order(Side side, int user_id, int price,
                                    int volume) const
      -> std::optional<std::string_view> {
    if (!user_cash.contains(user_id)) {
      return "User with id " + std::to_string(user_id) + " not found.";
    }

    switch (side) {
      case BUY: {
        std::scoped_lock lock(cash_mutex);
        if (price * volume > user_cash.at(user_id).buying_power) {
          return "Insufficient buying power for order.";
        }
        break;
      }
      case SELL:
        if (volume > user_assets.at(user_id).selling_power) {
          return "Insufficient asset " + to_string(asset) + " for order.";
        }
        break;
    }

    if (price <= 0) {
      return "Price must be positive";
    }

    if (volume <= 0) {
      return "Volume must be positive";
    }

    return {};
  }

  [[nodiscard]] auto execute_trade(Side taker_side, int maker_id, int taker_id,
                                   int price, int volume,
                                   int order_id) -> Trade {
    std::scoped_lock lock(cash_mutex);

    int order_cost = price * volume;
    switch (taker_side) {
      case BUY:
        user_cash[maker_id].amount_held += order_cost;
        user_cash[maker_id].buying_power += order_cost;
        user_cash[taker_id].amount_held -= order_cost;
        user_cash[taker_id].buying_power -= order_cost;

        user_assets[maker_id].amount_held -= volume;
        /*user_assets[maker_id].selling_power -= volume;*/
        user_assets[taker_id].amount_held += volume;
        user_assets[taker_id].selling_power += volume;
        break;
      case SELL:
        user_cash[maker_id].amount_held -= order_cost;
        /*user_cash[maker_id].buying_power -= order_cost;*/
        user_cash[taker_id].amount_held += order_cost;
        user_cash[taker_id].buying_power += order_cost;

        user_assets[maker_id].amount_held += volume;
        user_assets[maker_id].selling_power += volume;
        user_assets[taker_id].amount_held -= volume;
        user_assets[taker_id].selling_power -= volume;
        break;
    }
    return {taker_side == Side::BUY ? taker_id : maker_id,
            taker_side == Side::BUY ? maker_id : taker_id, price, volume,
            order_id};
  }

  [[nodiscard]] auto match_order(Side side, int user_id, int price, int& volume)
      -> std::optional<std::vector<Trade>> {
    std::vector<Trade> trades;

    auto begin = side == Side::BUY ? sell_orders.begin() : buy_orders.begin();
    auto end = side == Side::BUY ? sell_orders.upper_bound(price)
                                 : buy_orders.upper_bound(price);

    for (auto price_it = begin; price_it != end && volume > 0;) {
      auto& level = price_it->second;

      for (auto level_it = level.begin(); level_it != level.end() && volume > 0;
           ++level_it) {
        int trade_volume = std::min(volume, level_it->volume);

        volume -= trade_volume;
        level_it->volume -= trade_volume;

        trades.push_back(execute_trade(side, level_it->user_id, user_id,
                                       level_it->price, trade_volume,
                                       level_it->order_id));

        if (level_it->volume == 0) {
          level.erase(level_it);
        }
      }

      ++price_it;

      if (level.empty()) {
        switch (side) {
          case BUY:
            sell_orders.erase(std::prev(price_it));
            break;
          case SELL:
            buy_orders.erase(std::prev(price_it));
            break;
        }
      }
    }

    if (trades.empty()) {
      return {};
    }

    return trades;
  }

  [[nodiscard]] auto place_order(Side side, int user_id, int price,
                                 int volume) -> OrderResult {
    std::optional<std::string_view> error =
        validate_order(side, user_id, price, volume);
    if (error.has_value()) {
      return {error.value().data(), {}, {}};
    }

    std::optional<std::vector<Trade>> trades =
        match_order(side, user_id, price, volume);

    if (volume == 0) {
      return {{}, trades, {}};
    }

    switch (side) {
      case BUY: {
        std::scoped_lock lock(cash_mutex);
        user_cash[user_id].buying_power -= price * volume;
        buy_orders[price].emplace_back(side, user_id, price, volume,
                                       order_number);
        all_orders[order_number] = std::prev(buy_orders[price].end());
        break;
      }
      case SELL:
        user_assets[user_id].selling_power -= volume;
        sell_orders[price].emplace_back(side, user_id, price, volume,
                                        order_number);
        all_orders[order_number] = std::prev(sell_orders[price].end());
        break;
    }

    return {{}, trades, Order{side, user_id, price, volume, order_number++}};
  }

  [[nodiscard]] auto cancel_order(int order_id) -> std::optional<std::string> {
    if (!all_orders.contains(order_id)) {
      return "Error: Order not found.";
    }
    auto order_iter = all_orders[order_id];
    switch (order_iter->side) {
      case BUY:
        buy_orders[order_iter->price].erase(order_iter);
        if (buy_orders[order_iter->price].empty()) {
          buy_orders.erase(order_iter->price);
        }
        break;
      case SELL:
        sell_orders[order_iter->price].erase(order_iter);
        if (sell_orders[order_iter->price].empty()) {
          sell_orders.erase(order_iter->price);
        }
        break;
    }
    return {};
  }
};

auto inline operator<<(std::ostream& os,
                       const Exchange& exchange) -> std::ostream& {
  os << to_string(exchange.asset) << " exchange" << std::endl;
  os << "  BUY orders: " << std::endl;
  for (const auto& [price, orders] : exchange.buy_orders) {
    os << "    $" << price << ": " << std::endl;
    for (const Order& order : orders) {
      os << "      order_id: " << order.order_id
         << ", user_id: " << order.user_id << ", volume: " << order.volume
         << std::endl;
    }
  }
  os << "  SELL orders: " << std::endl;
  for (const auto& [price, orders] : exchange.sell_orders) {
    os << "    $" << price << ": " << std::endl;
    for (const Order& order : orders) {
      os << "      order_id: " << order.order_id
         << ", user_id: " << order.user_id << ", volume: " << order.volume
         << std::endl;
    }
  }
  return os;
}

#endif  // EXCHANGE_HPP
