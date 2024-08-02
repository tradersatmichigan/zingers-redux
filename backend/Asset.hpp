#ifndef ASSET_HPP
#define ASSET_HPP

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

#endif  // ASSET_HPP
