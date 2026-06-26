#include "forge/executor.hpp"

namespace forge {
namespace {
static_assert(sizeof(WorkerLease) >= sizeof(LeaseId));
}
}  // namespace forge
