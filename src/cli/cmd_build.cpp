#include "forge/client.hpp"

#include <iostream>

namespace forge::cli {

int cmd_build(int argc, char** argv) {
  (void)argc;
  (void)argv;
  WorkspaceCore workspace(".");
  DeterministicMockExecutor executor;
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
