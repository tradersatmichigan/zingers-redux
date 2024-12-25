#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>

#include "Exchange.hpp"
#include "Models.hpp"

std::unordered_map<uint32_t, Balance> Exchange::user_cash;
std::mutex Exchange::cash_mutex;

Exchange::Exchange(Asset asset) : asset(asset) {}

auto Exchange::register_user(uint16_t user_id) -> void {
  if (user_assets.contains(user_id)) {
    return;
  }
  user_assets[user_id] = {.held = 100, .power = 100};
  std::lock_guard lg(cash_mutex);
  if (user_cash.contains(user_id)) {
    return;
  }
  user_cash[user_id] = {.held = 1000, .power = 1000};
}

auto Exchange::validate_and_update(const Order& order) -> void {
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

auto Exchange::execute_trades(const std::vector<Trade>& trades) -> void {
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

auto Exchange::place_order(Order order) -> OrderResult {
  validate_and_update(order);
  order.id = num_orders++;
  auto& opposing_orders = order.side == BUY ? asks : bids;
  uint16_t start_price = order.side == BUY ? MIN_PRICE : MAX_PRICE;
  auto cond = [&order](uint16_t& curr_price) -> bool {
    return order.side == BUY ? curr_price <= order.price
                             : curr_price >= order.price;
  };
  auto next = [&order](uint16_t& curr_price) -> void {
    order.side == BUY ? ++curr_price : --curr_price;
  };
  std::vector<Trade> trades;
  for (uint16_t curr_price = start_price; cond(curr_price) && order.volume > 0;
       next(curr_price)) {
    auto& level = opposing_orders[curr_price];
    while (!level.empty() && order.volume > 0) {
      auto& other = *level.front();
      if (!other.valid) {
        level.pop_front();
        continue;
      }
      uint16_t traded_volume = std::min(other.volume, order.volume);
      other.volume -= traded_volume;
      order.volume -= traded_volume;
      trades.emplace_back(order.side == BUY ? order.user_id : other.user_id,
                          order.side == BUY ? other.user_id : order.user_id,
                          curr_price, traded_volume, other.id);
      if (other.volume == 0) {
        orders.erase(other.id);
        level.pop_front();
      }
    }
  }
  execute_trades(trades);
  if (order.volume > 0) {
    std::shared_ptr<Order> order_ptr = std::make_shared<Order>(order);
    orders[order.id] = order_ptr;
    if (order.side == BUY) {
      bids[order.price].push_back(order_ptr);
    } else {
      asks[order.price].push_back(order_ptr);
    }
    return {.unmatched_order = order, .trades = trades};
  }
  return {.unmatched_order = std::nullopt, .trades = trades};
}

auto Exchange::cancel_order(uint32_t order_id) -> Order {
  if (!orders.contains(order_id)) {
    throw std::runtime_error("Order not found.");
  }
  auto order = orders[order_id].lock();
  if (order == nullptr) {
    throw std::runtime_error("Order not found.");
  }
  order->valid = false;
  orders.erase(order_id);
  return *order;
}
