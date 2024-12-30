#pragma once

#include <cstdint>
#include <optional>
#include <vector>

using user_t = uint32_t;
using price_t = uint32_t;
using volume_t = uint32_t;
using order_t = uint32_t;

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
  Asset asset;
  Side side;
  user_t user_id;
  price_t price;
  volume_t volume;
  uint32_t id;
};

inline auto operator==(const Order& lhs, const Order& rhs) -> bool {
  return lhs.asset == rhs.asset && lhs.side == rhs.side &&
         lhs.user_id == rhs.user_id && lhs.price == rhs.price &&
         lhs.volume == rhs.volume && lhs.id == rhs.id;
}

struct Trade {
  user_t buyer_id;
  user_t seller_id;
  price_t price;
  volume_t volume;
  uint32_t matched_order_id;
};

inline auto operator==(const Trade& lhs, const Trade& rhs) -> bool {
  return lhs.buyer_id == rhs.buyer_id && lhs.seller_id == rhs.seller_id &&
         lhs.price == rhs.price && lhs.volume == rhs.volume &&
         lhs.matched_order_id == rhs.matched_order_id;
}

struct OrderResult {
  std::optional<Order> unmatched_order;
  std::vector<Trade> trades;
};

struct Balance {
  uint32_t held;
  uint32_t power;
};

struct OrderInfo {
  Side side : 1;
  price_t price : 15;
};

auto constexpr to_string(const Asset& asset) -> std::string {
  switch (asset) {
    case DRESSING:
      return "DRESSING";
    case RYE:
      return "RYE";
    case SWISS:
      return "SWISS";
    case PASTRAMI:
      return "PASTRAMI";
  }
}

auto constexpr to_string_lower(const Asset& asset) -> std::string {
  switch (asset) {
    case DRESSING:
      return "dressing";
    case RYE:
      return "rye";
    case SWISS:
      return "swiss";
    case PASTRAMI:
      return "pastrami";
  }
}

auto constexpr to_string(const Side& side) -> std::string {
  switch (side) {
    case BUY:
      return "BUY";
    case SELL:
      return "SELL";
  }
}

auto inline operator<<(std::ostream& os, const Asset& asset) -> std::ostream& {
  return os << to_string(asset);
}

auto inline operator<<(std::ostream& os, const Side& side) -> std::ostream& {
  return os << to_string(side);
}
