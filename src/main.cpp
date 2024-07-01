#include <algorithm>
#include <thread>

#include <glaze/glaze.hpp>
#include "App.h"

#include "Asset.hpp"
#include "Exchange.hpp"

// See https://github.com/stephenberry/glaze?tab=readme-ov-file#explicit-metadata
template <>
struct glz::meta<Order> {
  using T = Order;
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto value =
      object(&T::side, &T::user_id, &T::price, &T::volume);
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
};

auto run_asset_socket(Asset asset, Exchange& exchange) {
  auto* app = new uWS::SSLApp();

  auto on_open = [asset](uWS::WebSocket<true, true, UserData>* ws) {
    ws->send("Connected to exchange asset " + to_string(asset));
    ws->subscribe(DEFAULT_TOPIC);
  };

  auto on_message = [&app, &exchange](uWS::WebSocket<true, true, UserData>* ws,
                                      std::string_view message,
                                      uWS::OpCode opCode) {
    IncomingMessage message_data{};
    glz::error_ctx ec = glz::read_json(message_data, message);
    if (ec) {
      std::cout << glz::format_error(ec, message);
      return;
    }

    UserData* user_data = ws->getUserData();

    OrderResult result{};

    if (message_data.reg.has_value() && message_data.reg.value()) {
      if (!message_data.user_id.has_value()) {
        result.error = "Error: Must include user_id when registering.";
        ws->send(glz::write_json(result).value_or("Error encoding JSON."));
        return;
      }

      std::optional<std::string_view> err =
          exchange.register_user(message_data.user_id.value(), 1000, 100);

      if (err.has_value()) {
        result.error = err.value();
        ws->send(glz::write_json(result).value_or("Error encoding JSON."));
        return;
      }

      // TOOD: Probably want to remove this in production.
      ws->getUserData()->user_id = message_data.user_id.value();
      ws->send("Registered with user id: " +
               std::to_string(message_data.user_id.value()) + ".");
      return;
    }

    result = exchange.place_order(message_data.side.value(), user_data->user_id,
                                  message_data.price.value(),
                                  message_data.volume.value());

    app->publish(DEFAULT_TOPIC,
                 glz::write_json(result).value_or("Error encoding JSON."),
                 opCode);
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

  std::for_each(threads.begin(), threads.end(),
                [](std::thread* t) { t->join(); });
}
