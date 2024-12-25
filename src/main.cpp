#include <iostream>

#include <App.h>
#include <glaze/glaze.hpp>

#include "Exchange.hpp"
#include "Models.hpp"

auto main() -> int {
  uint16_t user1 = 1;
  uint16_t user2 = 2;

  Exchange exchange(PASTRAMI);
  exchange.register_user(user1);
  exchange.register_user(user2);
  return 0;
}
