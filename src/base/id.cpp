#include "forge/ids.hpp"

namespace forge {
namespace {
static_assert(sizeof(NodeId) == sizeof(std::uint64_t));
}
}  // namespace forge
