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
};

struct Order {
  int user_id;
  int price;
  int volume;

  Order(int user_id, int price, int volume)
      : user_id(user_id), price(price), volume(volume){};
};

struct OrderResult {
  std::optional<std::string_view> error;
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
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
  Asset asset;
  static inline std::unordered_map<int, Cash> user_cash;
  static inline std::mutex cash_mutex;
  std::unordered_map<int, AssetAmount> user_assets;
  std::map<int, std::deque<Order>> buy_orders;
  std::map<int, std::deque<Order>, std::greater<>> sell_orders;

  Exchange(Asset asset) : asset(asset) {}

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

    return {};
  }

  auto place_order(Side side, int user_id, int price,
                   int volume) -> OrderResult {
    /*std::optional<std::string_view> error =*/
    /*    validate_order(side, user_id, price, volume);*/
    /*if (error.has_value()) {*/
    /*  return {error, {}, {}};*/
    /*}*/

    switch (side) {
      case BUY:
        buy_orders[price].emplace_back(user_id, price, volume);
        break;
      case SELL:
        sell_orders[price].emplace_back(user_id, price, volume);
        break;
    }

    return {{}, {}, Order{user_id, price, volume}};
  }
};

auto inline operator<<(std::ostream& os,
                       const Exchange& exchange) -> std::ostream& {
  os << to_string(exchange.asset) << " exchange" << std::endl;
  os << "  BUY orders: " << std::endl;
  for (const auto& [price, orders] : exchange.buy_orders) {
    os << "    $" << price << ": " << std::endl;
    for (const Order& order : orders) {
      os << "      user_id: " << order.user_id << ", volume: " << order.volume
         << std::endl;
    }
  }
  os << "  SELL orders: " << std::endl;
  for (const auto& [price, orders] : exchange.sell_orders) {
    os << "    $" << price << ": " << std::endl;
    for (const Order& order : orders) {
      os << "      user_id: " << order.user_id << ", volume: " << order.volume
         << std::endl;
    }
  }
  return os;
}

#endif  // EXCHANGE_HPP
