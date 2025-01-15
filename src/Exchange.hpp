#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "Models.hpp"

struct Exchange {
  /* Used by all instances of Exchange */
  static inline std::unordered_map<uint32_t, Cash> user_cash;
  static inline std::mutex cash_mutex;
  static inline std::atomic_uint32_t order_number{0};

  /* Per-exchange information */
  Asset asset;
  std::unordered_map<uint32_t, AssetAmount> user_assets;
  std::array<std::list<Order>, MAX_PRICE + 1> buy_orders;
  std::array<std::list<Order>, MAX_PRICE + 1> sell_orders;
  std::unordered_map<uint32_t, std::list<Order>::iterator> all_orders;

  Exchange(Asset asset) : asset(asset) {}

  static auto verify_state(uint32_t user_id,
                           const std::vector<Exchange>& exchanges) -> void {
    std::lock_guard lg(cash_mutex);
    uint32_t expected_buying_power = user_cash.at(user_id).amount_held;
    for (const auto& exchange : exchanges) {
      for (uint32_t price = MIN_PRICE; price <= MAX_PRICE; ++price) {
        for (const auto& order : exchange.buy_orders[price]) {
          assert(order.price == price);
          if (order.user_id == user_id) {
            expected_buying_power -= order.volume * order.price;
          }
        }
      }
    }
    if (expected_buying_power != user_cash[user_id].buying_power) {
      std::cout << "user_id: " << user_id << '\n';
      std::cout << "Expected buying power: " << expected_buying_power << '\n';
      std::cout << "Actual buying power: " << user_cash[user_id].buying_power
                << '\n';
    }
    assert(expected_buying_power == user_cash[user_id].buying_power);
    for (const auto& exchange : exchanges) {
      uint32_t expected_selling_power =
          exchange.user_assets.at(user_id).amount_held;
      for (uint32_t price = MIN_PRICE; price <= MAX_PRICE; ++price) {
        for (const auto& order : exchange.sell_orders[price]) {
          assert(order.price == price);
          if (order.user_id == user_id) {
            expected_selling_power -= order.volume;
          }
        }
      }
      if (expected_selling_power !=
          exchange.user_assets.at(user_id).selling_power) {
        std::cout << "user_id: " << user_id << '\n';
        std::cout << "Expected selling power: " << expected_selling_power
                  << '\n';
        std::cout << "Actual selling power: "
                  << exchange.user_assets.at(user_id).selling_power << '\n';
      }
      assert(expected_selling_power ==
             exchange.user_assets.at(user_id).selling_power);
    }
  }

  static auto verify_state(const std::vector<Exchange>& exchanges) -> void {
    for (auto [user_id, _] : user_cash) {
      std::cout << user_id << '\n';
      verify_state(user_id, exchanges);
    }
  }

  auto register_user(uint32_t user_id, uint32_t cash, uint32_t assets) -> void {
    std::scoped_lock lock(cash_mutex);
    if (user_assets.contains(user_id)) {
      return;
    }
    if (!user_cash.contains(user_id)) {
      user_cash[user_id] = {.amount_held = cash, .buying_power = cash};
    }
    user_assets[user_id] = {.amount_held = assets, .selling_power = assets};
  }

  [[nodiscard]] auto validate_order(Side side, uint32_t user_id, uint32_t price,
                                    uint32_t volume) const
      -> std::optional<std::string> {
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

    if (price < MIN_PRICE || price > MAX_PRICE) {
      return "Price must be in range [1, 200] inclusive";
    }

    if (volume <= 0) {
      return "Volume must be positive";
    }

    return {};
  }

  [[nodiscard]] auto execute_trade(Side taker_side, uint32_t maker_id,
                                   uint32_t taker_id, uint32_t price,
                                   uint32_t volume, uint32_t order_id)
      -> Trade {
    std::scoped_lock lock(cash_mutex);

    uint32_t order_cost = price * volume;
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
    return {.buyer_id = taker_side == BUY ? taker_id : maker_id,
            .seller_id = taker_side == BUY ? maker_id : taker_id,
            .price = price,
            .volume = volume,
            .order_id = order_id};
  }

  [[nodiscard]] auto match_order(Side side, uint32_t user_id, uint32_t price,
                                 uint32_t& volume)
      -> std::optional<std::vector<Trade>> {
    std::vector<Trade> trades;

    auto& opposing_orders = side == BUY ? sell_orders : buy_orders;

    auto cmp = [side](uint32_t lhs, uint32_t rhs) -> bool {
      return side == BUY ? lhs >= rhs : lhs <= rhs;
    };

    for (uint32_t curr_price = side == BUY ? MIN_PRICE : MAX_PRICE;
         volume > 0 && cmp(price, curr_price);
         side == BUY ? ++curr_price : --curr_price) {
      auto& level = opposing_orders[curr_price];
      while (volume > 0 && !level.empty()) {
        auto iter = level.begin();
        uint32_t trade_volume = std::min(volume, iter->volume);
        volume -= trade_volume;
        iter->volume -= trade_volume;
        trades.push_back(execute_trade(side, iter->user_id, user_id,
                                       iter->price, trade_volume,
                                       iter->order_id));
        if (iter->volume == 0) {
          all_orders.erase(iter->order_id);
          level.erase(iter);
        }
      }
    }

    if (trades.empty()) {
      return {};
    }

    return trades;
  }

  [[nodiscard]] auto place_order(Side side, uint32_t user_id, uint32_t price,
                                 uint32_t volume) -> OrderResult {
    std::optional<std::string> error =
        validate_order(side, user_id, price, volume);
    if (error.has_value()) {
      return {.error = error, .trades = {}, .unmatched_order = {}};
    }

    std::optional<std::vector<Trade>> trades =
        match_order(side, user_id, price, volume);

    if (volume == 0) {
      return {.error = {}, .trades = trades, .unmatched_order = {}};
    }

    switch (side) {
      case BUY: {
        std::scoped_lock lock(cash_mutex);
        user_cash[user_id].buying_power -= price * volume;
        buy_orders[price].emplace_back(asset, side, user_id, price, volume,
                                       order_number);
        all_orders[order_number] = std::prev(buy_orders[price].end());
        break;
      }
      case SELL:
        user_assets[user_id].selling_power -= volume;
        sell_orders[price].emplace_back(asset, side, user_id, price, volume,
                                        order_number);
        all_orders[order_number] = std::prev(sell_orders[price].end());
        break;
    }

    return {.error = {},
            .trades = trades,
            .unmatched_order =
                Order{asset, side, user_id, price, volume, order_number++}};
  }

  [[nodiscard]] auto cancel_order(uint32_t order_id)
      -> std::optional<std::string> {
    if (!all_orders.contains(order_id)) {
      return "Order not found.";
    }
    auto order_iter = all_orders[order_id];
    switch (order_iter->side) {
      case BUY: {
        std::scoped_lock lock(cash_mutex);
        user_cash[order_iter->user_id].buying_power +=
            order_iter->price * order_iter->volume;
        buy_orders[order_iter->price].erase(order_iter);
        break;
      }
      case SELL:
        user_assets[order_iter->user_id].selling_power += order_iter->volume;
        sell_orders[order_iter->price].erase(order_iter);
        break;
    }
    all_orders.erase(order_id);
    return {};
  }
};

auto inline operator<<(std::ostream& os, const Exchange& exchange)
    -> std::ostream& {
  os << to_string(exchange.asset) << " exchange" << '\n';
  os << "  BUY orders: " << '\n';
  for (uint32_t price = MAX_PRICE; price >= MIN_PRICE; --price) {
    const auto& orders = exchange.buy_orders[price];
    if (orders.empty()) {
      continue;
    }
    os << "    $" << price << '\n';
    for (const Order& order : orders) {
      os << "      order_id: " << order.order_id
         << ", user_id: " << order.user_id << ", volume: " << order.volume
         << '\n';
    }
  }
  os << "  SELL orders: " << '\n';
  for (uint32_t price = MIN_PRICE; price <= MAX_PRICE; ++price) {
    const auto& orders = exchange.sell_orders[price];
    if (orders.empty()) {
      continue;
    }
    os << "    $" << price << '\n';
    for (const Order& order : orders) {
      os << "      order_id: " << order.order_id
         << ", user_id: " << order.user_id << ", volume: " << order.volume
         << '\n';
    }
  }
  return os;
}
