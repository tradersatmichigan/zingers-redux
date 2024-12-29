#include <glaze/glaze.hpp>

#include "Exchange.hpp"
#include "Types.hpp"

auto basic() -> void {
  user_t user1 = 1;
  user_t user2 = 2;

  Exchange exchange(PASTRAMI);
  exchange.register_user(user1);
  exchange.register_user(user2);

  price_t price1 = 10;
  volume_t volume1 = 5;
  Order order1 = {
      .asset = PASTRAMI,
      .side = BUY,
      .user_id = user1,
      .price = price1,
      .volume = volume1,
      .id = 10,
  };
  // std::cout << glz::write_json(order1).value_or("error parsing json") << '\n';

  auto [unmatched_order1, trades1] = exchange.place_order(order1);

  assert(trades1.empty());
  assert(unmatched_order1.has_value());
  Order expected_order_1 = {
      .asset = PASTRAMI,
      .side = BUY,
      .user_id = user1,
      .price = price1,
      .volume = volume1,
      .id = 0,
  };
  assert(unmatched_order1.value() == expected_order_1);
  assert(exchange.bids.size() == 1);
  assert(exchange.asks.size() == 0);
  assert(exchange.bids[price1].front() == expected_order_1);
  assert(user_cash[user1].power == STARTING_CASH - price1 * volume1);
  assert(user_cash[user1].held == STARTING_CASH);
  assert(exchange.user_assets[user1].power == STARTING_ASSETS);
  assert(exchange.user_assets[user1].held == STARTING_ASSETS);

  price_t price2 = 9;
  volume_t volume2 = 4;
  Order order2 = {
      .asset = PASTRAMI,
      .side = SELL,
      .user_id = user2,
      .price = price2,
      .volume = volume2,
      .id = 10,
  };

  auto [unmatched_order2, trades2] = exchange.place_order(order2);
  assert(trades2.size() == 1);
  Trade expected_trade2 = Trade{.buyer_id = user1,
                                .seller_id = user2,
                                .price = 10,
                                .volume = 4,
                                .matched_order_id = 0};
  assert(trades2[0] == expected_trade2);
  assert(!unmatched_order2.has_value());
  Order expected_order_2 = {
      .asset = PASTRAMI,
      .side = BUY,
      .user_id = user1,
      .price = price1,
      .volume = 1,
      .id = 0,
  };
  assert(exchange.bids.size() == 1);
  assert(exchange.asks.size() == 0);
  assert(exchange.bids[price1].front() == expected_order_2);

  assert(user_cash[user1].power == STARTING_CASH - price1 * volume1);
  assert(user_cash[user1].held == STARTING_CASH - price1 * (volume1 - 1));
  assert(exchange.user_assets[user1].power == STARTING_ASSETS + 4);
  assert(exchange.user_assets[user1].held == STARTING_ASSETS + 4);

  assert(user_cash[user2].power == STARTING_CASH + price1 * (volume1 - 1));
  assert(user_cash[user2].held == STARTING_CASH + price1 * (volume1 - 1));
  assert(exchange.user_assets[user2].power == STARTING_ASSETS - 4);
  assert(exchange.user_assets[user2].held == STARTING_ASSETS - 4);
}

auto main() -> int {
  basic();
  return 0;
}
