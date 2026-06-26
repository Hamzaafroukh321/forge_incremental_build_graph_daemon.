#include "forge/manifest.hpp"

#include <fstream>
#include <iostream>

namespace forge::cli {

int cmd_graph(int argc, char** argv) {
  if (argc >= 3 && std::string(argv[1]) == "apply") {
    std::ifstream input(argv[2]);
    if (!input) {
      std::cerr << "unable to open manifest: " << argv[2] << "\n";
      return 2;
    }
    auto manifest = parse_manifest(input);
    if (!manifest.ok()) {
      std::cerr << manifest.status().message << "\n";
      return 3;
    }
    WorkspaceCore workspace(".");
    auto applied = apply_manifest(workspace, manifest.value());
    if (!applied.ok()) {
      std::cerr << applied.status().message << "\n";
      return 3;
    }
    std::cout << "generation=" << workspace.graph().generation().value
              << " nodes=" << applied.value().nodes_by_name.size()
              << " edges=" << workspace.graph().edges().size()
              << " targets=" << applied.value().targets.size() << "\n";
    return 0;
  }

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
