#include "forge/client.hpp"
#include "forge/fipc.hpp"

#include <cstdint>
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  forge::FipcCodec codec;
  forge::FipcSession session;
  forge::WorkspaceCore workspace(".");
  forge::DeterministicMockExecutor executor;
  auto hello = codec.encode(forge::FipcFrame{forge::FrameType::hello, 0, 0, 0, 0, 0, {}});
  (void)session.feed(hello.data(), hello.size());
  std::vector<forge::NodeId> nodes;
  for (std::size_t i = 0; i < size && i < 256; ++i) {
    if ((data[i] & 1U) == 0U) {
      auto node = workspace.put_node("pipe" + std::to_string(i), forge::ActionSpec{"mock", {{"byte", std::to_string(data[i])}}});
      if (node.ok()) {
        nodes.push_back(node.value());
      }
    } else if (nodes.size() > 1) {
      (void)workspace.put_edge(nodes.back(), nodes[(data[i] / 2U) % nodes.size()]);
      (void)workspace.build({nodes.back()}, executor);
    }
  }
  return 0;
}

#ifndef FORGE_USE_LIBFUZZER
int main() {
  const std::uint8_t seed[] = {0, 2, 4, 5, 7, 9};
  LLVMFuzzerTestOneInput(seed, sizeof(seed));
  std::cout << "forge_build_pipeline_fuzz smoke ok\n";
  return 0;
}
#endif
