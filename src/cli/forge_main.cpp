#include "forge/client.hpp"

#include <iostream>

namespace forge::cli {
int cmd_build(int argc, char** argv);
int cmd_graph(int argc, char** argv);
int cmd_status(int argc, char** argv);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: forge <build [manifest] [targets...]|graph apply <manifest>|status [--socket PATH]|demo>\n";
    return 2;
  }
  const std::string command = argv[1];
  if (command == "build" || command == "demo") {
    return forge::cli::cmd_build(argc - 1, argv + 1);
  }
  if (command == "graph") {
    return forge::cli::cmd_graph(argc - 1, argv + 1);
  }
  if (command == "status") {
    return forge::cli::cmd_status(argc - 1, argv + 1);
  }
  std::cerr << "unknown command: " << command << "\n";
  return 2;
}
