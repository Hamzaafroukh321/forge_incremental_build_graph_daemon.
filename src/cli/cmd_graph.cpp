#include "forge/client.hpp"

#include <iostream>

namespace forge::cli {

int cmd_graph(int argc, char** argv) {
  (void)argc;
  (void)argv;
  WorkspaceCore workspace(".");
  auto node = workspace.put_node("sample", ActionSpec{"mock", {}});
  if (!node.ok()) {
    std::cerr << node.status().message << "\n";
    return 3;
  }
  std::cout << "generation=" << workspace.graph().generation().value << " node=" << node.value().value << "\n";
  return 0;
}

}  // namespace forge::cli
