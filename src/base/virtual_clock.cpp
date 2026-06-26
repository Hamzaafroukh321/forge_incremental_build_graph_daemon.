#include "forge/workspace.hpp"

namespace forge {
namespace {
static_assert(sizeof(VirtualClock) <= sizeof(std::uint64_t));
}
}  // namespace forge
