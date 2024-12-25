#pragma once

#include <cstdint>
#include <optional>
#include <vector>

enum Asset : uint8_t {
  DRESSING = 0,
  RYE = 1,
  SWISS = 2,
  PASTRAMI = 3,
};

enum Side : uint8_t {
  BUY = 0,
  SELL = 1,
};

struct Order {
  bool valid : 1;
  Asset asset : 7;
  Side side;
  uint16_t user_id;
  uint16_t price;
  uint16_t volume;
  uint32_t id;
};

struct Trade {
  uint16_t buyer_id;
  uint16_t seller_id;
  uint16_t price;
  uint16_t volume;
  uint32_t matched_order_id;
};

struct OrderResult {
  std::optional<Order> unmatched_order;
  std::vector<Trade> trades;
};

struct Balance {
  uint16_t held;
  uint16_t power;
};
