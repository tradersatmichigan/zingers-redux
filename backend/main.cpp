#include <unistd.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <random>
#include <string>
#include <thread>

#include <glaze/glaze.hpp>
#include <unordered_map>
#include <unordered_set>
#include "App.h"
#include "WebSocketProtocol.h"

#include "Asset.hpp"
#include "Exchange.hpp"

// See https://github.com/stephenberry/glaze?tab=readme-ov-file#explicit-metadata
template <>
struct glz::meta<Order> {
  using T = Order;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto value = object(&T::asset, &T::side, &T::user_id,
                                       &T::price, &T::volume, &T::order_id);
};

constexpr int NUM_ASSETS = 4;
constexpr std::string_view DEFAULT_TOPIC = "default";

struct UserData {
  int user_id{-1};
};

struct IncomingMessage {
  std::optional<bool> reg;
  std::optional<int> user_id;
  std::optional<Asset> asset;
  std::optional<Side> side;
  std::optional<int> price;
  std::optional<int> volume;
  std::optional<bool> cancel;
  std::optional<int> order_id;
};

struct GameState {
  std::optional<std::string> error;
  std::unordered_map<int, Order> orders;
  int cash{0};
  int buying_power{0};
  std::vector<int> assets_held;
  std::vector<int> selling_power;
};

struct LoginResult {
  std::optional<std::string> error;
  std::optional<int> user_id;
};

const std::vector<int> STARTING_CASH = {1000, 1000, 1020, 1000};
const std::vector<int> STARTING_ASSETS = {200, 100, 66, 50};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
uint8_t next_assignment = DRESSING;

auto handle_register_message(Exchange& exchange,
                             uWS::WebSocket<true, true, UserData>* ws,
                             const IncomingMessage& message_data,
                             uWS::OpCode op_code) -> void {
  OrderResult result{};
  if (!message_data.user_id.has_value()) {
    result.error = "Must include user_id when registering.";
    ws->send(glz::write_json(result).value_or("Error encoding JSON."), op_code);
    return;
  }

  exchange.register_user(message_data.user_id.value(), 1000, 100);

  ws->getUserData()->user_id = message_data.user_id.value();
  ws->send("Registered with user id: " +
               std::to_string(message_data.user_id.value()) + ".",
           op_code);
}

auto handle_cancel_message(Exchange& exchange, uWS::SSLApp* app,
                           uWS::WebSocket<true, true, UserData>* ws,
                           const IncomingMessage& message_data,
                           uWS::OpCode op_code) -> void {
  CancelResult result{};
  if (!message_data.order_id.has_value()) {
    result.error = "error: Must include order_id when canceling an order.";
    ws->send(glz::write_json(result).value_or("Error encoding JSON."), op_code);
    return;
  }
  std::optional<std::string> error =
      exchange.cancel_order(message_data.order_id.value());
  if (error.has_value()) {
    result.error = error;
    ws->send(glz::write_json(result).value_or("Error encoding JSON."), op_code);
    return;
  }
  result.order_id = message_data.order_id.value();
  app->publish(DEFAULT_TOPIC,
               glz::write_json(result).value_or("Error encoding JSON."),
               op_code);
}

auto run_asset_socket(Asset asset, Exchange& exchange) {
  auto* app = new uWS::SSLApp();

  auto on_open = [asset](uWS::WebSocket<true, true, UserData>* ws) {
    ws->send("Connected to exchange asset " + to_string(asset),
             uWS::OpCode::TEXT);
    ws->subscribe(DEFAULT_TOPIC);
  };

  auto on_message = [&app, &exchange](uWS::WebSocket<true, true, UserData>* ws,
                                      std::string_view message,
                                      uWS::OpCode op_code) {
    IncomingMessage message_data{};
    glz::error_ctx ec = glz::read_json(message_data, message);
    if (ec) {
      std::cout << glz::format_error(ec, message);
      return;
    }

    UserData* user_data = ws->getUserData();

    OrderResult result{};

    if (message_data.reg.has_value() && message_data.reg.value()) {
      handle_register_message(exchange, ws, message_data, op_code);
      std::cout << exchange << std::endl;
      return;
    }

    if (user_data->user_id == -1) {
      result.error = "error: Not registered on this exchange";
      ws->send(glz::write_json(result).value_or("Error encoding JSON."),
               op_code);
      return;
    }

    if (message_data.cancel.has_value() && message_data.cancel.value()) {
      handle_cancel_message(exchange, app, ws, message_data, op_code);
      std::cout << exchange << std::endl;
      return;
    }

    result = exchange.place_order(message_data.side.value(), user_data->user_id,
                                  message_data.price.value(),
                                  message_data.volume.value());

    app->publish(DEFAULT_TOPIC,
                 glz::write_json(result).value_or("Error encoding JSON."),
                 op_code);

    std::cout << exchange << std::endl;
  };

  app->ws<UserData>("/asset/" + to_string_lower(asset),
                    {
                        .open = on_open,
                        .message = on_message,
                    })
      .listen(9001 + asset, [asset](auto* listen_s) {
        if (listen_s) {
          std::cout << "Listening on port " << 9001 + asset << std::endl;
        }
      });

  app->run();

  delete app;

  uWS::Loop::get()->free();
}

auto run_api(std::vector<Exchange>& exchanges) -> void {
  std::default_random_engine e1(42);
  std::uniform_int_distribution<int> id_generator(0, INT_MAX);

  auto generate_user_id = [&e1, &id_generator]() -> int {
    return id_generator(e1);
  };

  std::unordered_map<std::string, std::string> passwords;
  std::unordered_map<std::string, int> username_to_id;
  std::unordered_set<int> user_ids;

  auto handle_state_request = [&exchanges](uWS::HttpResponse<true>* res,
                                           uWS::HttpRequest* req) {
    GameState state;
    if (req->getHeader("user-id").empty()) {
      state.error = "user_id not set";
      res->end(glz::write_json(state).value_or("Error encoding JSON."));
      return;
    }
    int user_id = std::stoi(req->getHeader("user-id").data());
    std::unordered_map<int, std::deque<Order>::iterator> order_iters;
    for (const auto& exchange : exchanges) {
      if (!exchange.user_assets.contains(user_id)) {
        state.error = "User with user_id: " + std::to_string(user_id) +
                      " not registered on exchange " +
                      to_string(exchange.asset);
        res->end(glz::write_json(state).value_or("Error encoding JSON."));
        return;
      }
    }
    state.cash = Exchange::user_cash[user_id].amount_held;
    state.buying_power = Exchange::user_cash[user_id].buying_power;
    for (const auto& exchange : exchanges) {
      for (auto [order_id, order_iter] : exchange.all_orders) {
        state.orders.emplace(order_id, *order_iter);
      }
      state.assets_held.push_back(exchange.user_assets.at(user_id).amount_held);
      state.selling_power.push_back(
          exchange.user_assets.at(user_id).selling_power);
    }
    res->end(glz::write_json(state).value_or("Error encoding JSON."));
  };

  auto handle_login_request =
      [&exchanges, &passwords, &user_ids, &username_to_id, &generate_user_id](
          uWS::HttpResponse<true>* res, uWS::HttpRequest* req) {
        LoginResult result;
        std::string username = req->getHeader("username").data();
        std::string password = req->getHeader("password").data();
        if (passwords.contains(username)) {
          if (passwords[username] == password) {
            result.user_id = username_to_id[username];
            res->end(glz::write_json(result).value_or("Error encoding JSON."));
          } else {
            result.error = "incorrect password";
            res->end(glz::write_json(result).value_or("Error encoding JSON."));
          }
          return;
        }
        passwords[username] = password;
        int user_id = generate_user_id();
        while (user_ids.contains(user_id)) {
          user_id = generate_user_id();
        }
        result.user_id = user_id;
        username_to_id[username] = user_id;
        for (auto& exchange : exchanges) {
          int cash = STARTING_CASH[next_assignment];
          int assets = next_assignment == exchange.asset
                           ? STARTING_ASSETS[next_assignment]
                           : 0;
          exchange.register_user(user_id, cash, assets);
        }
        next_assignment = (next_assignment + 1) % 4;
        res->end(glz::write_json(result).value_or("Error encoding JSON."));
      };

  uWS::SSLApp()
      .get("/api/state", handle_state_request)
      .post("/api/login", handle_login_request)
      .get("/api/redirect",
           [](uWS::HttpResponse<true>* res, uWS::HttpRequest* /*req*/) {
             res->writeStatus("302");
             res->writeHeader("location", "https://google.com");
             res->end();
           })
      .listen(3000,
              [](auto* listen_socket) {
                if (listen_socket) {
                  std::cout << "Listening on port " << 3000 << std::endl;
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

  std::vector<std::thread*> threads(NUM_ASSETS);
  for (uint8_t i = 0; i < NUM_ASSETS; ++i) {
    auto asset = static_cast<Asset>(i);
    auto& exchange = exchanges[i];
    threads[i] = new std::thread(
        [asset, &exchange]() { run_asset_socket(asset, exchange); });
  }

  run_api(exchanges);

  std::for_each(threads.begin(), threads.end(),
                [](std::thread* t) { t->join(); });
}
