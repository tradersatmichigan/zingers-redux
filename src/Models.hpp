#ifndef MODELS_HPP
#define MODELS_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

auto constexpr value(Asset asset) -> uint32_t {
  switch (asset) {
    case DRESSING:
      return 10;
    case RYE:
      return 20;
    case SWISS:
      return 30;
    case PASTRAMI:
      return 40;
    default:
      std::unreachable();
  }
}

constexpr uint32_t RUEBEN_VALUE = 100;

enum Side : uint8_t {
  BUY = 0,
  SELL = 1,
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

struct Cash {
  uint32_t amount_held;
  uint32_t buying_power;
};

struct AssetAmount {
  uint32_t amount_held;
  uint32_t selling_power;
};

struct OrderResult {
  std::optional<std::string> error;
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
};

// API/Websocket message types

struct SocketData {
  uint32_t user_id{0};
  bool registered{false};
};

enum MessageType : uint8_t {
  REGISTER = 0,
  ORDER = 1,
  CANCEL = 2,
  ERROR = 3,
};

struct IncomingMessage {
  std::optional<MessageType> type;
  // register
  std::optional<uint32_t> user_id;
  std::optional<std::string_view> username;
  // order
  std::optional<Asset> asset;
  std::optional<Side> side;
  std::optional<uint32_t> price;
  std::optional<uint32_t> volume;
  // cancel
  std::optional<uint32_t> order_id;
};

struct OutgoingMessage {
  std::optional<MessageType> type;
  std::optional<std::string> error;
  // register
  std::optional<uint32_t> user_id;
  std::optional<std::string_view> username;
  // order
  std::optional<std::vector<Trade>> trades;
  std::optional<Order> unmatched_order;
  // cancel
  std::optional<uint32_t> order_id;
};

struct GameState {
  std::optional<std::string> error;
  std::unordered_map<uint32_t, Order> orders;
  uint32_t cash{0};
  uint32_t buying_power{0};
  std::vector<uint32_t> assets_held;
  std::vector<uint32_t> selling_power;
};

#endif  // MODELS_HPP
