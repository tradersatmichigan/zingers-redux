#include <App.h>
#include <glaze/glaze.hpp>

struct UserData {};

constexpr int PORT = 9001;
constexpr int HEARTBEAT_TIME = 10;
constexpr std::string DEFAULT_TOPIC = "default";

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
  std::cout << "closing connection\n";
}

auto main() -> int {
  auto* app = new uWS::App();
  app->ws<UserData>("/*", {.compression = uWS::DISABLED,
                           .idleTimeout = HEARTBEAT_TIME,
                           .open = on_open,
                           .message = on_message(app),
                           .close = on_close})
      .listen(PORT,
              [](us_listen_socket_t* listenSocket) {
                if (listenSocket != nullptr) {
                  std::cout << "Listening on port " << PORT << '\n';
                } else {
                  std::cout << "Failed to load certs or to bind to port\n";
                }
              })
      .run();
}
