#ifndef ASSET_HPP
#define ASSET_HPP

#include <cstdint>
#include <string>

enum Asset : uint8_t {
  // Maybe change to 0, 1, 2, 3
  // Using 10, 20, 30, 40 so we can use static_cast<int>(asset) to get value
  DRESSING = 10,
  RYE = 20,
  SWISS = 30,
  PASTRAMI = 40,
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
  }
}

#endif  // ASSET_HPP
