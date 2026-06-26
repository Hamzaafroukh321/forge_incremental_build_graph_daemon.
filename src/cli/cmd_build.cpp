#include "forge/manifest.hpp"

#include <fstream>
#include <iostream>

namespace forge::cli {

int cmd_build(int argc, char** argv) {
  WorkspaceCore workspace(".");
  DeterministicMockExecutor executor;
  if (argc >= 2) {
    std::ifstream input(argv[1]);
    if (!input) {
      std::cerr << "unable to open manifest: " << argv[1] << "\n";
      return 2;
    }
    auto manifest = parse_manifest(input);
    if (!manifest.ok()) {
      std::cerr << manifest.status().message << "\n";
      return 3;
    }
    auto applied = apply_manifest(workspace, manifest.value());
    if (!applied.ok()) {
      std::cerr << applied.status().message << "\n";
      return 3;
    }
    std::vector<NodeId> targets;
    if (argc > 2) {
      for (int i = 2; i < argc; ++i) {
        const auto found = applied.value().nodes_by_name.find(argv[i]);
        if (found == applied.value().nodes_by_name.end()) {
          std::cerr << "unknown target: " << argv[i] << "\n";
          return 3;
        }
        targets.push_back(found->second);
      }
    } else {
      targets = applied.value().targets;
    }
    auto outcome = workspace.build(targets, executor);
    if (!outcome.ok()) {
      std::cerr << outcome.status().message << "\n";
      return 3;
    }
    for (const auto& event : outcome.value().events) {
      std::cout << event.message << "\n";
    }
    std::cout << "artifacts=" << outcome.value().artifacts.size() << "\n";
    return 0;
  }

  auto lib = workspace.put_node("lib", ActionSpec{"mock-lib", {{"mode", "debug"}}});
  auto app = workspace.put_node("app", ActionSpec{"mock-app", {{"mode", "debug"}}});
  if (!lib.ok() || !app.ok()) {
    std::cerr << "failed to create demo graph\n";
    return 3;
  }
  auto edge = workspace.put_edge(app.value(), lib.value());
  if (!edge.ok()) {
    std::cerr << edge.status().message << "\n";
    return 3;
  }
  auto outcome = workspace.build({app.value()}, executor);
  if (!outcome.ok()) {
    std::cerr << outcome.status().message << "\n";
    return 3;
  }
  for (const auto& event : outcome.value().events) {
    std::cout << event.message << "\n";
  }
  std::cout << "artifacts=" << outcome.value().artifacts.size() << "\n";
  return 0;
}

}  // namespace forge::cli
