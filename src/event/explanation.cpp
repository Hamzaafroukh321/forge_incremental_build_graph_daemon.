#include "forge/event.hpp"

namespace forge {
namespace {
static_assert(static_cast<int>(EventKind::succeeded) > static_cast<int>(EventKind::queued));
}
}  // namespace forge
