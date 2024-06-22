#include <iostream>
#include "Asset.hpp"
#include "Exchange.hpp"

auto main() -> int {
  Exchange exchange(Asset::DRESSING);
  exchange.place_order(Side::BUY, 0, 10, 1);
  exchange.place_order(Side::SELL, 0, 11, 1);
  std::cout << exchange;
  return 0;
}
