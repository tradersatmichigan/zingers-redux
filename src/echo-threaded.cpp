#include <algorithm>
#include <thread>
#include "App.h"

constexpr int NUM_ASSETS = 4;

auto main() -> int {
  struct PerSocketData {};

  std::vector<std::thread*> threads(NUM_ASSETS);

  for (size_t i = 0; i < NUM_ASSETS; ++i) {
    threads[i] = new std::thread([i]() {
      uWS::App()
          .ws<PerSocketData>(
              "/*", {
                        .open =
                            [](auto* ws) {
                              std::cout << "Connected to thread "
                                        << std::this_thread::get_id()
                                        << std::endl;
                              ws->send("Connected");
                            },
                        .message =
                            [](auto* ws, std::string_view message,
                               uWS::OpCode opCode) {
                              std::cout << std::this_thread::get_id()
                                        << " received message: " << message
                                        << std::endl;
                              ws->send(message, opCode);
                            },
                    })
          .listen(9001 + static_cast<int>(i),
                  [i](auto* listen_socket) {
                    if (listen_socket) {
                      std::cout << "Thread " << std::this_thread::get_id()
                                << " listening on port "
                                << 9001 + static_cast<int>(i) << std::endl;
                    } else {
                      std::cout << "Thread " << std::this_thread::get_id()
                                << " failed to listen on port 9001"
                                << std::endl;
                    }
                  })
          .run();
    });
  }

  std::for_each(threads.begin(), threads.end(),
                [](std::thread* t) { t->join(); });
}
