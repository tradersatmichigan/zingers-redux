#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include "Models.hpp"

constexpr uint16_t MIN_PRICE = 1;
constexpr uint16_t MAX_PRICE = 200;
constexpr uint16_t MIN_VOLUME = 1;
constexpr uint16_t MAX_VOLUME = 200;

class Exchange {
  static std::unordered_map<uint32_t, Balance> user_cash;
  static std::mutex cash_mutex;

  Asset asset;
  uint32_t num_orders{0};
  std::array<std::deque<std::shared_ptr<Order>>, MAX_PRICE + 1> bids;
  std::array<std::deque<std::shared_ptr<Order>>, MAX_PRICE + 1> asks;
  std::unordered_map<uint16_t, Balance> user_assets;
  std::unordered_map<uint32_t, std::weak_ptr<Order>> orders;

  /*
   * Validate that 'order' is valid in terms of price, volume, and
   * buying/selling power required. Also updates buying/selling power.
   *
   * Throws on invalid `order`.
   */
  auto validate_and_update(const Order& order) -> void;

  /*
   * Execute `trades`, update buyer and sellers accounts accordingly.
   */
  auto execute_trades(const std::vector<Trade>& trades) -> void;

 public:
  Exchange() = delete;
  Exchange(Asset asset);

  auto register_user(uint16_t user_id) -> void;

  /*
   * Place an order. Will attempt to match with existing orders.
   *
   * Returns `std::vector<Trade>` of trades caused by the order, and
   * `std::optional<Order>` if any of the order volume is unmatched.
   *
   * Throws on invalid `order`.
   */
  auto place_order(Order order) -> OrderResult;

  /*
  * Cancel's order with `order_id`, returns the order if it exists.
  *
  * Throws error if `order_id` is not found.
  */
  auto cancel_order(uint32_t order_id) -> Order;
};
