#include "forge/client.hpp"

namespace forge::integration {

bool build_flow_smoke() {
  WorkspaceCore workspace(".");
  DeterministicMockExecutor executor;
  auto source = workspace.put_node("source", ActionSpec{"source", {}});
  auto output = workspace.put_node("output", ActionSpec{"output", {}});
  if (!source.ok() || !output.ok()) {
    return false;
  }
  if (!workspace.put_edge(output.value(), source.value()).ok()) {
    return false;
  }
  auto built = workspace.build({output.value()}, executor);
  return built.ok() && built.value().artifacts.size() == 2;
}

}  // namespace forge::integration
