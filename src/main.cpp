#include <unistd.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>

#include <glaze/glaze.hpp>
#include "App.h"
#include "WebSocketProtocol.h"

#include "Exchange.hpp"
#include "Models.hpp"

// See https://github.com/stephenberry/glaze?tab=readme-ov-file#explicit-metadata
template <>
struct glz::meta<Order> {
  using T = Order;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto value = object(&T::asset, &T::side, &T::user_id,
                                       &T::price, &T::volume, &T::order_id);
};

constexpr uint32_t NUM_ASSETS = 4;
constexpr std::string_view DEFAULT_TOPIC = "default";

const std::vector<uint32_t> STARTING_CASH = {1000, 1000, 1020, 1000};
const std::vector<uint32_t> STARTING_ASSETS = {200, 100, 66, 50};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
uint8_t next_assignment = DRESSING;

auto handle_register_message(Exchange& exchange,
                             uWS::WebSocket<true, true, SocketData>* ws,
                             const IncomingMessage& message_data,
                             uWS::OpCode op_code) -> void {
  if (ws->getUserData()->registered) {
    return;
  }
  OutgoingMessage outgoing{};
  if (!message_data.user_id.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = "Must include user_id when registering.";
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }

  exchange.register_user(message_data.user_id.value(), 1000, 100);

  ws->getUserData()->user_id = message_data.user_id.value();
  ws->getUserData()->registered = true;
  ws->send("Registered with user id: " +
               std::to_string(message_data.user_id.value()) + ".",
           op_code);
}

auto handle_cancel_message(Exchange& exchange, uWS::SSLApp* app,
                           uWS::WebSocket<true, true, SocketData>* ws,
                           const IncomingMessage& incoming,
                           uWS::OpCode op_code) -> void {
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
  if (error.has_value()) {
    outgoing.type = ERROR;
    outgoing.error = error.value();
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }
  outgoing.type = CANCEL;
  outgoing.order_id = incoming.order_id.value();
  app->publish(DEFAULT_TOPIC,
               glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
}

auto handle_order_message(Exchange& exchange, uWS::SSLApp* app,
                          uWS::WebSocket<true, true, SocketData>* ws,
                          const IncomingMessage& incoming,
                          uWS::OpCode op_code) -> void {
  OutgoingMessage outgoing{};
  SocketData* user_data = ws->getUserData();
  if (!user_data->registered) {
    outgoing.type = ERROR;
    outgoing.error =
        "Not registered on exchange" + to_string_lower(exchange.asset);
    ws->send(glz::write_json(outgoing).value_or("Error encoding JSON."),
             op_code);
    return;
  }

  OrderResult order_result =
      exchange.place_order(incoming.side.value(), user_data->user_id,
                           incoming.price.value(), incoming.volume.value());
  outgoing.type = ORDER;
  outgoing.trades = std::move(order_result.trades);
  outgoing.unmatched_order = order_result.unmatched_order;

  app->publish(DEFAULT_TOPIC,
               glz::write_json(outgoing).value_or("Error encoding JSON."),
               op_code);
}

auto run_asset_socket(Asset asset, Exchange& exchange) {
  auto* app = new uWS::SSLApp();

  auto on_open = [asset](uWS::WebSocket<true, true, SocketData>* ws) {
    ws->send("Connected to exchange asset " + to_string(asset),
             uWS::OpCode::TEXT);
    ws->subscribe(DEFAULT_TOPIC);
  };

  auto on_message = [&app, &exchange](
                        uWS::WebSocket<true, true, SocketData>* ws,
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
        handle_register_message(exchange, ws, incoming, op_code);
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

    std::cout << exchange << std::endl;
  };

  app->ws<SocketData>("/asset/" + to_string_lower(asset),
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

auto run_api(const std::vector<Exchange>& exchanges) -> void {
  auto handle_state_request = [&exchanges](uWS::HttpResponse<true>* res,
                                           uWS::HttpRequest* req) {
    GameState state;
    if (req->getHeader("user-id").empty()) {
      state.error = "user_id not set";
      res->end(glz::write_json(state).value_or("Error encoding JSON."));
      return;
    }
    uint32_t user_id =
        static_cast<uint32_t>(std::stoul(req->getHeader("user-id").data()));
    std::unordered_map<uint32_t, std::deque<Order>::iterator> order_iters;
    for (const auto& exchange : exchanges) {
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

  uWS::SSLApp()
      .get("/api/game/get_state", handle_state_request)
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
