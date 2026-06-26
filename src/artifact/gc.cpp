#include "forge/artifact.hpp"

namespace forge {

std::size_t ArtifactStore::gc() {
  std::size_t removed = 0;
  for (auto it = objects_.begin(); it != objects_.end();) {
    if (it->second.leases == 0) {
      it = objects_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}

}  // namespace forge
