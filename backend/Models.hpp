#ifndef MODELS_HPP
#define MODELS_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum Asset : uint8_t {
  DRESSING = 0,
  RYE = 1,
  SWISS = 2,
  PASTRAMI = 3,
};

auto constexpr to_string(Asset asset) -> std::string {
  switch (asset) {
    case DRESSING:
      return "DRESSING";
    case RYE:
      return "RYE";
    case SWISS:
      return "SWISS";
    case PASTRAMI:
      return "PASTRAMI";
    default:
      std::unreachable();
  }
}

auto constexpr to_string_lower(Asset asset) -> std::string {
  switch (asset) {
    case DRESSING:
      return "dressing";
    case RYE:
      return "rye";
    case SWISS:
      return "swiss";
    case PASTRAMI:
      return "pastrami";
    default:
      std::unreachable();
  }
}

enum Side : bool {
  BUY = false,
  SELL = true,
};

struct Trade {
  uint32_t buyer_id;
  uint32_t seller_id;
  uint32_t price;
  uint32_t volume;
  uint32_t order_id;
};

struct Order {
  Asset asset;
  Side side;
  uint32_t user_id;
  uint32_t price;
  uint32_t volume;
  uint32_t order_id;

  Order(Asset asset, Side side, uint32_t user_id, uint32_t price,
        uint32_t volume, uint32_t order_id)
      : asset(asset),
        side(side),
        user_id(user_id),
        price(price),
        volume(volume),
        order_id(order_id) {};
};

struct OrderResult {
  std::optional<std::string> error;
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
};

struct CancelResult {
  std::optional<std::string> error;
  std::optional<uint32_t> order_id;
};

struct Cash {
  uint32_t amount_held;
  uint32_t buying_power;
};

struct AssetAmount {
  uint32_t amount_held;
  uint32_t selling_power;
};

#endif  // MODELS_HPP
