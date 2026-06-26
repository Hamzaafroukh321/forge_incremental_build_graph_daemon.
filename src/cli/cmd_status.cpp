#include "forge/client.hpp"
#include "forge/daemon.hpp"

#include <iostream>

namespace forge::cli {

int cmd_status(int argc, char** argv) {
  if (argc == 3 && std::string{argv[1]} == "--socket") {
    UnixSocketClient client(argv[2]);
    auto status = client.status();
    if (!status.ok()) {
      std::cerr << status.status().message << "\n";
      return 10;
    }
    std::cout << status.value();
    return 0;
  }
  if (argc != 1) {
    std::cerr << "usage: forge status [--socket PATH]\n";
    return 2;
  }
  WorkspaceCore workspace(".");
  std::cout << "state=initialized generation=" << workspace.graph().generation().value << "\n";
  return 0;
}

}  // namespace forge::cli
