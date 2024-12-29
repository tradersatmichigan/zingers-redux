#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "Types.hpp"

constexpr price_t MIN_PRICE = 1;
constexpr price_t MAX_PRICE = 20000;
constexpr volume_t MIN_VOLUME = 1;
constexpr volume_t MAX_VOLUME = 20000;

static std::unordered_map<user_t, Balance> user_cash{};
static std::mutex cash_mutex{};

static constexpr price_t STARTING_CASH = 1000;
static constexpr volume_t STARTING_ASSETS = 1000;

using OrderMap =
    std::map<price_t, std::deque<Order>, std::function<bool(price_t, price_t)>>;

struct Exchange {
  Asset asset;
  order_t num_orders{0};

  OrderMap bids{std::less<int>{}};
  OrderMap asks{std::greater<int>{}};
  std::unordered_map<user_t, Balance> user_assets;
  std::unordered_map<order_t, OrderInfo> order_info;

  Exchange() = delete;
  Exchange(Asset asset) : asset(asset) {}

  /*
   * Validate that 'order' is valid in terms of price, volume, and
   * buying/selling power required. Also updates buying/selling power.
   *
   * Throws on invalid `order`.
   */
  auto validate_and_update(const Order& order) -> void {
    if (order.asset != asset) {
      throw std::runtime_error("Tried to place order on wrong exchange.");
    }
    if (order.price < MIN_PRICE || order.price > MAX_PRICE) {
      throw std::runtime_error("Price must be in [1, 200] inclusive.");
    }
    if (order.volume < MIN_VOLUME || order.volume > MAX_VOLUME) {
      throw std::runtime_error("Volume must be in [1, 200] inclusive.");
    }
    if (order.side == BUY) {
      std::lock_guard lg(cash_mutex);
      if (order.price * order.volume > user_cash[order.user_id].power) {
        throw std::runtime_error("Insufficient buying power for order.");
      }
      user_cash[order.user_id].power -= order.price * order.volume;
    } else {
      if (order.volume > user_assets[order.user_id].power) {
        throw std::runtime_error("Insufficient selling power for order.");
      }
      user_assets[order.user_id].power -= order.volume;
    }
  }

  /*
   * Execute `trades`, update buyer and sellers accounts accordingly.
   */
  auto execute_trades(const std::vector<Trade>& trades) -> void {
    if (trades.empty()) {
      return;
    }
    std::lock_guard lg(cash_mutex);
    for (const auto& trade : trades) {
      user_assets[trade.buyer_id].held += trade.volume;
      user_assets[trade.buyer_id].power += trade.volume;
      user_assets[trade.seller_id].held -= trade.volume;
      // already accounted for in `validate_and_update`
      // user_assets[trade.seller_id].power -= trade.volume;

      user_cash[trade.buyer_id].held -= trade.volume * trade.price;
      user_cash[trade.seller_id].held += trade.volume * trade.price;
      user_cash[trade.seller_id].power += trade.volume * trade.price;
      // already accounted for in `validate_and_update`
      // user_cash[trade.buyer_id].power -= trade.volume * trade.price;
    }
  }

  auto register_user(user_t user_id) -> void {
    if (user_assets.contains(user_id)) {
      throw std::runtime_error(
          "User with this user_id already exists on this exchange.");
    }
    user_assets[user_id] = {.held = STARTING_ASSETS, .power = STARTING_ASSETS};
    std::lock_guard lg(cash_mutex);
    if (!user_cash.contains(user_id)) {
      std::cout << "Giving user " << user_id << " starting cash.\n";
      user_cash[user_id] = {.held = STARTING_CASH, .power = STARTING_CASH};
    }
    std::cout << "Registering user " << user_id << " on " << asset
              << " exchange.\n";
  }

  /*
   * Place an order. Will attempt to match with existing orders.
   *
   * Returns `std::vector<Trade>` of trades caused by the order, and
   * `std::optional<Order>` if any of the order volume is unmatched.
   *
   * Throws on invalid `order`.
   */
  [[nodiscard]] auto place_order(Order order) -> OrderResult {
    validate_and_update(order);
    order.id = num_orders++;
    auto& opposing_orders = order.side == BUY ? asks : bids;
    auto cond = [&order](const price_t& curr_price) -> bool {
      return order.side == BUY ? curr_price <= order.price
                               : curr_price >= order.price;
    };
    std::vector<Trade> trades;
    for (auto iter = opposing_orders.begin();
         iter != opposing_orders.end() && cond(iter->first);) {
      auto curr_price = iter->first;
      auto& level = opposing_orders[curr_price];
      while (!level.empty() && order.volume > 0) {
        auto& other = level.front();
        volume_t traded_volume = std::min(other.volume, order.volume);
        other.volume -= traded_volume;
        order.volume -= traded_volume;
        trades.emplace_back(order.side == BUY ? order.user_id : other.user_id,
                            order.side == BUY ? other.user_id : order.user_id,
                            curr_price, traded_volume, other.id);
        if (other.volume == 0) {
          order_info.erase(other.id);
          level.pop_front();
        }
      }
      ++iter;
      if (level.empty()) {
        opposing_orders.erase(curr_price);
      }
    }
    execute_trades(trades);
    if (order.volume > 0) {
      order_info[order.id] = {.side = order.side, .price = order.price};
      if (order.side == BUY) {
        bids[order.price].push_back(order);
      } else {
        asks[order.price].push_back(order);
      }
      return {.unmatched_order = order, .trades = trades};
    }
    return {.unmatched_order = std::nullopt, .trades = trades};
  }

  /*
  * Cancels order with `order_id`, returns the order if it exists.
  *
  * Throws error if `order_id` is not found.
  */
  auto cancel_order(order_t order_id, user_t user_id) -> void {
    if (!order_info.contains(order_id)) {
      throw std::runtime_error("Order not found.");
    }
    auto info = order_info[order_id];
    if (info.price < MIN_PRICE || info.price > MAX_PRICE) {
      throw std::runtime_error("Existing order has invalid price");
    }
    auto& orders = (info.side == BUY ? bids : asks)[info.price];
    auto iter =
        std::ranges::find_if(orders, [order_id](const Order& other) -> bool {
          return other.id == order_id;
        });
    if (iter->user_id != user_id) {
      throw std::runtime_error("user_id mismatch when cancelling order");
    }
    if (iter->side == BUY) {
      std::lock_guard lg(cash_mutex);
      user_cash[user_id].power += iter->price * iter->volume;
    } else {
      user_assets[user_id].power += iter->volume;
    }
    order_info.erase(order_id);
    orders.erase(iter);
    if (orders.empty()) {
      (info.side == BUY ? bids : asks).erase(info.price);
    }
  }
};
