#include "forge/client.hpp"

#include <cstdint>
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  forge::WorkspaceCore workspace(".");
  forge::DeterministicMockExecutor executor;
  std::vector<forge::NodeId> nodes;
  for (std::size_t i = 0; i < size && i < 512; ++i) {
    switch (data[i] % 5U) {
      case 0: {
        auto node = workspace.put_node("n" + std::to_string(i), forge::ActionSpec{"mock" + std::to_string(data[i] % 7U), {}});
        if (node.ok()) {
          nodes.push_back(node.value());
        }
        break;
      }
      case 1:
        if (nodes.size() > 1) {
          (void)workspace.put_edge(nodes[data[i] % nodes.size()], nodes[(data[i] / 2U) % nodes.size()]);
        }
        break;
      case 2:
        workspace.fs().write("p" + std::to_string(data[i] % 11U), forge::Bytes{data[i]});
        (void)workspace.file_event("p" + std::to_string(data[i] % 11U), forge::FileEventKind::modify);
        break;
      case 3:
        if (!nodes.empty()) {
          (void)workspace.build({nodes[data[i] % nodes.size()]}, executor);
        }
        break;
      default:
        workspace.clock().advance(data[i]);
        break;
    }
  }
  return 0;
}

#ifndef FORGE_USE_LIBFUZZER
int main() {
  const std::uint8_t seed[] = {0, 0, 1, 3, 2, 4};
  LLVMFuzzerTestOneInput(seed, sizeof(seed));
  std::cout << "forge_daemon_event_sequence_fuzz smoke ok\n";
  return 0;
}
#endif
