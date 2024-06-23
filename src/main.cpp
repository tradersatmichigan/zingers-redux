#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

#include "Asset.hpp"
#include "Exchange.hpp"

auto generate_user_ids(size_t num_users) -> std::vector<int> {
  std::default_random_engine e1(42);
  std::uniform_int_distribution<int> id_generator(1, 1'000'000);

  std::unordered_set<int> set;
  while (set.size() < num_users) {
    set.insert(id_generator(e1));
  }

  return {set.begin(), set.end()};
}

auto generate_orders(const std::vector<int>& user_ids,
                     size_t num_orders) -> std::vector<Order> {
  std::default_random_engine e1(42);
  std::uniform_int_distribution<int> side_generator(0, 1);
  std::uniform_int_distribution<size_t> id_generator(0, user_ids.size() - 1);
  std::uniform_int_distribution<int> price_generator(1, 50);
  std::uniform_int_distribution<int> volume_generator(1, 50);

  std::vector<Order> orders;
  orders.reserve(num_orders);

  for (size_t i = 0; i < num_orders; ++i) {
    Side side = static_cast<Side>(side_generator(e1));
    int user_id = user_ids[id_generator(e1)];
    int price = price_generator(e1);
    int volume = volume_generator(e1);
    orders.emplace_back(side, user_id, price, volume);
  }

  return orders;
}

auto benchmark(Exchange& exchange, size_t num_users,
               size_t num_orders) -> void {
  std::vector<int> user_ids = generate_user_ids(num_users);

  auto t_start = std::chrono::high_resolution_clock::now();
  for (int user_id : user_ids) {
    std::optional<std::string_view> err = exchange.register_user(user_id);
    if (err.has_value()) {
      std::cout << err.value() << std::endl;
    }
  }
  auto t_end = std::chrono::high_resolution_clock::now();

  std::cout << "Registering users took: "
            << std::chrono::duration<double, std::milli>(t_end - t_start)
            << std::endl;

  std::vector<Order> orders = generate_orders(user_ids, num_orders);

  t_start = std::chrono::high_resolution_clock::now();
  for (Order order : orders) {
    auto res = exchange.place_order(order.side, order.user_id, order.price,
                                    order.volume);
  }
  t_end = std::chrono::high_resolution_clock::now();

  std::cout << "Placing orders took: "
            << std::chrono::duration<double, std::milli>(t_end - t_start)
            << std::endl;
}

auto example(Exchange& exchange) -> void {
  auto err = exchange.register_user(0);
  if (err.has_value()) {
    std::cout << err.value() << std::endl;
  }
  err = exchange.register_user(1);
  if (err.has_value()) {
    std::cout << err.value() << std::endl;
  }

  OrderResult res = exchange.place_order(Side::BUY, 0, 10, 1);
  if (res.error.has_value()) {
    std::cout << res.error.value() << std::endl;
  }
  res = exchange.place_order(Side::SELL, 1, 10, 2);
  if (res.error.has_value()) {
    std::cout << res.error.value() << std::endl;
  }
  if (res.trades.has_value()) {
    for (const Trade& trade : res.trades.value()) {
      std::cout << "buyer_id: " << trade.buyer_id
                << ", seller_id: " << trade.seller_id << ", price "
                << trade.price << ", volume: " << trade.volume << std::endl;
    }
  }

  std::cout << exchange;
}

auto main() -> int {
  Exchange exchange(Asset::DRESSING);

  benchmark(exchange, 100, 1'000'000);

  return 0;
}
