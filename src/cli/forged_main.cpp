#include "forge/daemon.hpp"

#include <iostream>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  forge::Daemon daemon(".");
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
