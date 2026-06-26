#include "forge/executor.hpp"

namespace forge {
namespace {
bool discovery_edges_are_bounded(const WorkerResult& result) {
  return result.discovered_edges.size() < 200000;
}
static_assert(noexcept(std::declval<std::vector<std::pair<NodeId, NodeId>>&>().size()));
}
}  // namespace forge
