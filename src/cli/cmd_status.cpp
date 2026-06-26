#include "forge/client.hpp"

#include <iostream>

namespace forge::cli {

int cmd_status(int argc, char** argv) {
  (void)argc;
  (void)argv;
  WorkspaceCore workspace(".");
  std::cout << "state=initialized generation=" << workspace.graph().generation().value << "\n";
  return 0;
}

}  // namespace forge::cli
