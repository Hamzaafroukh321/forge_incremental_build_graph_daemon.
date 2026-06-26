#include "forge/executor.hpp"

namespace forge {
namespace {
static_assert(noexcept(std::declval<std::vector<std::pair<NodeId, NodeId>>&>().size()));
}
}  // namespace forge
