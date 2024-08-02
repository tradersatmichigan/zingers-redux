#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Asset.hpp"
#include "Exchange.hpp"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex output_mutex;

auto generate_user_ids(size_t num_users) -> std::vector<int> {
  std::default_random_engine e1(42);
  std::uniform_int_distribution<int> id_generator(1, 1'000'000);

  std::unordered_set<int> set;
  while (set.size() < num_users) {
    set.insert(id_generator(e1));
  }

  return {set.begin(), set.end()};
}

auto generate_orders(Asset asset, const std::vector<int>& user_ids,
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
    orders.emplace_back(asset, side, user_id, price, volume, i);
  }

  return orders;
}

auto benchmark(Exchange& exchange, const std::vector<int>& user_ids,
               size_t num_orders) -> void {
  auto t_start = std::chrono::high_resolution_clock::now();
  for (int user_id : user_ids) {
    exchange.register_user(user_id, 100'000'000, 100'000'000);
  }
  auto t_end = std::chrono::high_resolution_clock::now();

  {
    std::scoped_lock lock(output_mutex);
    std::cout << "Registering users took: "
              << std::chrono::duration<double, std::milli>(t_end - t_start)
              << std::endl;
  }

  std::vector<Order> orders =
      generate_orders(exchange.asset, user_ids, num_orders);

  t_start = std::chrono::high_resolution_clock::now();
  for (Order order : orders) {
    auto res = exchange.place_order(order.side, order.user_id, order.price,
                                    order.volume);
    if (res.error.has_value()) {
      std::scoped_lock lock(output_mutex);
      std::cout << res.error.value() << std::endl;
    }
  }
  t_end = std::chrono::high_resolution_clock::now();

  {
    std::scoped_lock lock(output_mutex);
    std::cout << "Placing orders took: "
              << std::chrono::duration<double, std::milli>(t_end - t_start)
              << std::endl;
  }
}

auto benchmark_to_csv(Exchange& exchange, const std::vector<int>& user_ids,
                      size_t num_orders) -> void {
  for (int user_id : user_ids) {
    exchange.register_user(user_id, 1'000'000, 100'000);
  }
  {
    std::scoped_lock lock(output_mutex);
    std::cout << "asset,side,user_id,price,volume,asset,buyer_id,seller_id,"
                 "price,volume"
              << std::endl;
  }

  std::vector<Order> orders =
      generate_orders(exchange.asset, user_ids, num_orders);

  for (Order order : orders) {
    auto res = exchange.place_order(order.side, order.user_id, order.price,
                                    order.volume);
    {
      std::scoped_lock lock(output_mutex);
      std::cout << to_string(exchange.asset) << ","
                << (order.side == BUY ? "BUY" : "SELL") << "," << order.user_id
                << "," << order.price << "," << order.volume << ",,,,,"
                << std::endl;
    }
    if (res.error.has_value()) {
      std::scoped_lock lock(output_mutex);
      std::cout << res.error.value() << std::endl;
    }
    if (res.trades.has_value()) {
      std::scoped_lock lock(output_mutex);
      for (const Trade& trade : res.trades.value()) {
        std::cout << ",,,,," << to_string(exchange.asset) << ","
                  << trade.buyer_id << "," << trade.seller_id << ","
                  << trade.price << "," << trade.volume << std::endl;
      }
    }
  }
}

auto example(Exchange& exchange) -> void {
  exchange.register_user(0, 1000, 100);
  exchange.register_user(1, 1000, 100);

  OrderResult res = exchange.place_order(BUY, 0, 10, 1);
  if (res.error.has_value()) {
    std::cout << res.error.value() << std::endl;
  }
  res = exchange.place_order(SELL, 1, 10, 2);
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

constexpr size_t NUM_ASSETS = 1;

auto main() -> int {
  std::vector<std::thread*> threads(NUM_ASSETS);

  std::vector<int> user_ids = generate_user_ids(100);

  for (size_t i = 0; i < NUM_ASSETS; ++i) {
    threads[i] = new std::thread([i, &user_ids]() {
      Exchange exchange(static_cast<Asset>(i));

      benchmark_to_csv(exchange, user_ids, 1'000'000);
    });
  }

  std::for_each(threads.begin(), threads.end(),
                [](std::thread* t) { t->join(); });

  return 0;
}
