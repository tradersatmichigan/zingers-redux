#ifndef MODELS_HPP
#define MODELS_HPP

#include <cstdint>
#include <string>
#include <utility>

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
  int buyer_id;
  int seller_id;
  int price;
  int volume;
  int order_id;
};

struct Order {
  Asset asset;
  Side side;
  int user_id;
  int price;
  int volume;
  int order_id;

  Order(Asset asset, Side side, int user_id, int price, int volume,
        int order_id)
      : asset(asset),
        side(side),
        user_id(user_id),
        price(price),
        volume(volume),
        order_id(order_id){};
};

struct OrderResult {
  std::optional<std::string> error;
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
};

struct CancelResult {
  std::optional<std::string> error;
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

#endif  // MODELS_HPP
