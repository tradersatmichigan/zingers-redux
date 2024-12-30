#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>

#include <App.h>
#include <glaze/glaze.hpp>

#include "Types.hpp"

struct UserData {};

constexpr int PORT = 9001;
constexpr int HEARTBEAT_TIME = 10;
constexpr std::string DEFAULT_TOPIC = "default";
constexpr int NUM_THREADS = 4;

std::mutex cout_mutex;

auto on_open(uWS::WebSocket<false, true, UserData>* ws) -> void {
  ws->subscribe(DEFAULT_TOPIC);
}

auto on_message(uWS::App* app) {
  return [app](uWS::WebSocket<false, true, UserData>* ws,
               std::string_view message, uWS::OpCode opCode) {
    // sending a message back to the sender
    ws->send(message, opCode);
    // broadcasting a message
    app->publish(DEFAULT_TOPIC, message, opCode);
  };
}

auto on_close(uWS::WebSocket<false, true, UserData>* /*ws*/, int /*code*/,
              std::string_view /*message*/) {
  std::lock_guard cout_lock(cout_mutex);
  std::cout << "closing connection\n";
}

auto run_websocket(int asset_number) -> void {
  assert(0 <= asset_number && asset_number < 4);
  auto asset = static_cast<Asset>(asset_number);
  auto port = PORT + asset_number;
  auto app = new uWS::App();
  auto path = "/asset/" + to_string_lower(asset);
  {
    std::lock_guard cout_lock(cout_mutex);
    std::cout << "port: " << port << ", path: " << path << '\n';
  }
  app->ws<UserData>(path, {.compression = uWS::DISABLED,
                           .idleTimeout = HEARTBEAT_TIME,
                           .open = on_open,
                           .message = on_message(app),
                           .close = on_close});
  app->listen(port, [port](us_listen_socket_t* listenSocket) {
    if (listenSocket != nullptr) {
      std::lock_guard cout_lock(cout_mutex);
      std::cout << "Listening on port " << port << '\n';
    } else {
      std::lock_guard cout_lock(cout_mutex);
      std::cout << "Failed to load certs or to bind to port\n";
    }
  });
  app->run();

  delete app;

  uWS::Loop::get()->free();
}

auto main() -> int {
  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);
  for (size_t i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back(run_websocket, i);
  }

  std::ranges::for_each(threads, [](std::thread& t) { t.join(); });
}
