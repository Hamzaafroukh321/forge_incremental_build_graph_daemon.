#include "forge/daemon.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::filesystem::path socket_path;
  bool socket_mode = false;
  bool once = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--socket" && i + 1 < argc) {
      socket_path = argv[++i];
      socket_mode = true;
    } else if (arg == "--once") {
      once = true;
    } else {
      std::cerr << "usage: forged [--socket PATH] [--once]\n";
      return 2;
    }
  }
  forge::Daemon daemon(".");
  if (socket_mode) {
    forge::UnixSocketDaemon server(socket_path, daemon);
    auto started = server.start();
    if (!started.ok()) {
      std::cerr << started.status().message << "\n";
      return 10;
    }
    std::cout << "forged socket=" << socket_path.string() << "\n";
    do {
      auto served = server.serve_once();
      if (!served.ok()) {
        std::cerr << served.status().message << "\n";
        return 10;
      }
    } while (!once);
    auto stopped = server.stop();
    if (!stopped.ok()) {
      std::cerr << stopped.status().message << "\n";
      return 10;
    }
    return 0;
  }
  auto started = daemon.start();
  if (!started.ok()) {
    std::cerr << started.status().message << "\n";
    return 10;
  }
  std::cout << "forged state=running\n";
  auto stopped = daemon.stop();
  if (!stopped.ok()) {
    std::cerr << stopped.status().message << "\n";
    return 10;
  }
  return 0;
}
