#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "App.h"
#include "WebSocketProtocol.h"
#include <glaze/glaze.hpp>

#include "Exchange.hpp"
#include "Models.hpp"
#include "libusockets.h"

// See
// https://github.com/stephenberry/glaze?tab=readme-ov-file#explicit-metadata
template <> struct glz::meta<Order> {
  using T = Order;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto value = object(&T::asset, &T::side, &T::user_id,
                                       &T::price, &T::volume, &T::order_id);
};

constexpr uint32_t NUM_ASSETS = 4;
constexpr std::string_view DEFAULT_TOPIC = "default";

const std::vector<uint32_t> STARTING_CASH = {30000, 30000, 30000, 27400};
// const uint32_t STARTING_CASH = 10000;
const std::vector<std::vector<uint32_t>> STARTING_ASSETS = {
    {1000, 101, 66, 50}, // 16000
    {200, 501, 66, 50},  // 16000
    {201, 100, 333, 50},
    {200, 101, 66, 250},
};
std::atomic<uint8_t> next_assignment = DRESSING;
std::mutex assignments_mutex;
std::unordered_map<uint32_t, uint8_t> assignments;
std::mutex cout_mutex;
std::array<us_listen_socket_t *, 4> asset_sockets = {nullptr};
us_listen_socket_t *api_socket = nullptr;
bool accepting = false;

auto handle_register_message(
    Exchange &exchange, std::unordered_map<uint32_t, std::string> &usernames,
    uWS::WebSocket<true, true, SocketData> *ws, const IncomingMessage &incoming,
    uWS::OpCode op_code) -> void {
  if (ws->getUserData()->registered) {
    return;
  }
  OutgoingMessage outgoing{};
  if (!incoming.user_id.has_value() || !incoming.username.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = "Must include user_id and username when registering.";
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }

  {
    std::scoped_lock lock(assignments_mutex);
    if (!assignments.contains(incoming.user_id.value())) {
      assignments[incoming.user_id.value()] = next_assignment;
      next_assignment = (next_assignment + 1) % 4;
    }
  }

  exchange.register_user(
      incoming.user_id.value(),
      STARTING_CASH[assignments[incoming.user_id.value()]],
      STARTING_ASSETS[assignments[incoming.user_id.value()]][exchange.asset]);
  usernames[incoming.user_id.value()] = incoming.username.value();

  ws->getUserData()->user_id = incoming.user_id.value();
  ws->getUserData()->registered = true;
  outgoing.type = REGISTER;
  outgoing.user_id = incoming.user_id;
  outgoing.username = incoming.username.value();
  ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."), op_code);
}

auto handle_cancel_message(Exchange &exchange, uWS::SSLApp *app,
                           uWS::WebSocket<true, true, SocketData> *ws,
                           const IncomingMessage &incoming,
                           uWS::OpCode op_code) -> void {
  if (!accepting) {
    return;
  }
  OutgoingMessage outgoing{};
  if (!incoming.order_id.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = "Must include order_id when canceling an order.";
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }
  std::optional<std::string> error =
      exchange.cancel_order(incoming.order_id.value());
  outgoing.order_id = incoming.order_id;
  if (error.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = error.value();
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }
  outgoing.type = CANCEL;
  app->publish(DEFAULT_TOPIC,
               glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
}

auto handle_order_message(Exchange &exchange, uWS::SSLApp *app,
                          uWS::WebSocket<true, true, SocketData> *ws,
                          const IncomingMessage &incoming,
                          uWS::OpCode op_code) -> void {
  if (!accepting) {
    return;
  }
  OutgoingMessage outgoing{};
  SocketData *user_data = ws->getUserData();
  if (!user_data->registered) {
    outgoing.type = ERROR;
    outgoing.error =
        "Not registered on exchange" + to_string_lower(exchange.asset);
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }

  if (!incoming.side.has_value() || !incoming.price.has_value() ||
      !incoming.volume.has_value()) {
    outgoing.type = ERROR;
    outgoing.error =
        "Must specify side, price, and volume when placing an order";
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }

  OrderResult order_result =
      exchange.place_order(incoming.side.value(), user_data->user_id,
                           incoming.price.value(), incoming.volume.value());
  if (order_result.error.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = order_result.error.value();
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }
  outgoing.type = ORDER;
  outgoing.trades = std::move(order_result.trades);
  outgoing.unmatched_order = order_result.unmatched_order;

  app->publish(DEFAULT_TOPIC,
               glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
}

auto run_asset_socket(Asset asset, Exchange &exchange,
                      std::unordered_map<uint32_t, std::string> &usernames) {
  auto *app = new uWS::SSLApp();

  auto on_open = [](uWS::WebSocket<true, true, SocketData> *ws) {
    ws->subscribe(DEFAULT_TOPIC);
  };

  auto on_message = [&app, &exchange, &usernames](
                        uWS::WebSocket<true, true, SocketData> *ws,
                        std::string_view message, uWS::OpCode op_code) {
    IncomingMessage incoming{};
    glz::error_ctx ec = glz::read_json(incoming, message);

    if (ec) {
      OutgoingMessage outgoing{};
      outgoing.type = ERROR;
      outgoing.error = glz::format_error(ec, message);
      ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
      return;
    }

    if (!incoming.type.has_value()) {
      OutgoingMessage outgoing{};
      outgoing.type = ERROR;
      outgoing.error = "Message must have typed attached.";
      ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
      return;
    }

    switch (incoming.type.value()) {
    case REGISTER:
      handle_register_message(exchange, usernames, ws, incoming, op_code);
      break;
    case ORDER:
      handle_order_message(exchange, app, ws, incoming, op_code);
      break;
    case CANCEL:
      handle_cancel_message(exchange, app, ws, incoming, op_code);
      break;
    case ERROR:
      break;
    }

    // std::cout << exchange << '\n';
  };

  app->ws<SocketData>("/asset/" + to_string_lower(asset),
                      {
                          .idleTimeout = 10,
                          .open = on_open,
                          .message = on_message,
                      })
      .listen(9001 + asset, [asset](auto *listen_s) {
        if (listen_s) {
          asset_sockets[asset] = listen_s;
          // std::lock_guard lg(cout_mutex);
          // std::cout << "Listening on port " << 9001 + asset << '\n';
        }
      });

  app->run();

  delete app;

  uWS::Loop::get()->free();
}

auto handle_state_request(const std::vector<Exchange> &exchanges) {
  return [&exchanges](uWS::HttpResponse<true> *res,
                      uWS::HttpRequest *req) -> void {
    GameState state;
    if (req->getHeader("user-id").empty()) {
      state.error = "user_id not set";
      res->end(glz::write_json(state).value_or("Error encoding JSON."));
      return;
    }
    uint32_t user_id =
        static_cast<uint32_t>(std::stoul(req->getHeader("user-id").data()));
    std::unordered_map<uint32_t, std::deque<Order>::iterator> order_iters;
    for (const auto &exchange : exchanges) {
      if (!exchange.user_assets.contains(user_id)) {
        state.error = "User with user_id: " + std::to_string(user_id) +
                      " not registered on exchange " +
                      to_string(exchange.asset);
        res->end(glz::write_json(state).value_or("Error encoding JSON."));
        return;
      }
    }
    state.cash = Exchange::user_cash.at(user_id).amount_held;
    state.buying_power = Exchange::user_cash.at(user_id).buying_power;
    for (const auto &exchange : exchanges) {
      for (auto [order_id, order_iter] : exchange.all_orders) {
        state.orders.emplace(order_id, *order_iter);
      }
      state.assets_held.push_back(exchange.user_assets.at(user_id).amount_held);
      state.selling_power.push_back(
          exchange.user_assets.at(user_id).selling_power);
    }
    res->end(glz::write_json(state).value_or("Error encoding JSON."));
  };
};

auto get_portfolio_value(const std::vector<Exchange> &exchanges,
                         uint32_t user_id) -> uint32_t {
  if (!Exchange::user_cash.contains(user_id)) {
    return 0;
  }
  uint32_t portfolio_value = Exchange::user_cash.at(user_id).amount_held;
  std::optional<uint32_t> ruebens = {};
  for (const auto &exchange : exchanges) {
    if (exchange.user_assets.contains(user_id)) {
      portfolio_value +=
          exchange.user_assets.at(user_id).amount_held * value(exchange.asset);
      if (ruebens.has_value()) {
        ruebens = std::min(ruebens.value(),
                           exchange.user_assets.at(user_id).amount_held);
      } else {
        ruebens = exchange.user_assets.at(user_id).amount_held;
      }
    }
  }

  if (ruebens.has_value()) {
    portfolio_value += ruebens.value() * RUEBEN_VALUE;
  }

  return portfolio_value;
}

auto handle_leaderboard_request(
    const std::vector<Exchange> &exchanges,
    const std::unordered_map<uint32_t, std::string> &usernames) {
  return [&exchanges, &usernames](uWS::HttpResponse<true> *res,
                                  uWS::HttpRequest * /*req*/) -> void {
    // auto get_portfolio_value = [&exchanges](uint32_t user_id) -> uint32_t {
    //   if (!Exchange::user_cash.contains(user_id)) {
    //     return 0;
    //   }
    //   uint32_t portfolio_value = Exchange::user_cash.at(user_id).amount_held;
    //   std::optional<uint32_t> ruebens = {};
    //   for (const auto& exchange : exchanges) {
    //     if (exchange.user_assets.contains(user_id)) {
    //       portfolio_value += exchange.user_assets.at(user_id).amount_held *
    //                          value(exchange.asset);
    //       if (ruebens.has_value()) {
    //         ruebens = std::min(ruebens.value(),
    //                            exchange.user_assets.at(user_id).amount_held);
    //       } else {
    //         ruebens = exchange.user_assets.at(user_id).amount_held;
    //       }
    //     }
    //   }
    //
    //   if (ruebens.has_value()) {
    //     portfolio_value += ruebens.value() * RUEBEN_VALUE;
    //   }
    //
    //   return portfolio_value;
    // };

    std::unordered_map<std::string, uint32_t> leaderboard;
    leaderboard.reserve(Exchange::user_cash.size());
    for (const auto [user_id, _] : Exchange::user_cash) {
      if (!usernames.contains(user_id)) {
        std::cout << "user_id: " << user_id << " has no username";
      }
      leaderboard[usernames.at(user_id)] =
          get_portfolio_value(exchanges, user_id);
    }
    res->end(glz::write_json(leaderboard).value_or("Error encoding JSON."));
  };
}

auto run_api(const std::vector<Exchange> &exchanges,
             const std::unordered_map<uint32_t, std::string> &usernames)
    -> void {

  uWS::SSLApp()
      .get("/api/game/get_state", handle_state_request(exchanges))
      .get("/api/game/get_leaderboard",
           handle_leaderboard_request(exchanges, usernames))
      .listen(3000,
              [](us_listen_socket_t *listen_socket) {
                if (listen_socket) {
                  api_socket = listen_socket;
                  // std::lock_guard lg(cout_mutex);
                  // std::cout << "Listening on port " << 3000 << '\n';
                }
              })
      .run();
}

auto main() -> int {
  std::vector<Exchange> exchanges;
  exchanges.reserve(NUM_ASSETS);
  for (uint8_t i = 0; i < NUM_ASSETS; ++i) {
    exchanges.emplace_back(static_cast<Asset>(i));
  }

  std::vector<std::thread *> threads(NUM_ASSETS);
  std::unordered_map<uint32_t, std::string> usernames;
  for (uint8_t i = 0; i < NUM_ASSETS; ++i) {
    auto asset = static_cast<Asset>(i);
    auto &exchange = exchanges[i];
    threads[i] = new std::thread([asset, &exchange, &usernames]() {
      run_asset_socket(asset, exchange, usernames);
    });
  }

  std::thread api_thread(
      [&exchanges, &usernames]() { run_api(exchanges, usernames); });
  std::cout << "Type 'start' to end the game and display final leaderboard\n";
  std::cout << "% ";
  std::string cmd;
  while (std::cin >> cmd && cmd != "start") {
  }
  accepting = true;

  std::cout << "Type 'end' to end the game and display final leaderboard\n";
  std::cout << "% ";
  cmd = "";
  while (std::cin >> cmd && cmd != "end") {
  }
  accepting = false;
  std::ranges::for_each(asset_sockets, [](us_listen_socket_t *listen_s) {
    us_listen_socket_close(0, listen_s);
  });
  us_listen_socket_close(0, api_socket);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);

  std::vector<std::pair<uint32_t, std::string>> leaderboard;
  leaderboard.reserve(Exchange::user_cash.size());
  for (const auto [user_id, _] : Exchange::user_cash) {
    leaderboard.emplace_back(get_portfolio_value(exchanges, user_id),
                             usernames[user_id]);
  }
  std::ranges::sort(leaderboard, std::greater<>{});
  for (const auto &[portfolio_value, username] : leaderboard) {
    std::cout << username << ": " << portfolio_value << '\n';
  }

  std::ranges::for_each(threads, [](std::thread *t) { t->join(); });
  api_thread.join();
}
