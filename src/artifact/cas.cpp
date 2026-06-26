#include "forge/artifact.hpp"

namespace forge {

Result<ArtifactKey> ArtifactStore::publish(ArtifactWriter writer) {
  auto object = writer.finish();
  if (!object.ok()) {
    return object.status();
  }
  const ArtifactKey key = object.value().key;
  if (digest_bytes("artifact", object.value().bytes) != key.digest) {
    return Status::error(ErrorCode::artifact, "cas", "artifact digest mismatch");
  }
  objects_[key] = std::move(object.value());
  return key;
}

Result<Bytes> ArtifactStore::read(const ArtifactKey& key) const {
  auto it = objects_.find(key);
  if (it == objects_.end() || !it->second.durable) {
    return Status::error(ErrorCode::artifact, "cas", "artifact not found");
  }
  return it->second.bytes;
}

Result<void> ArtifactStore::acquire(const ArtifactKey& key) {
  auto it = objects_.find(key);
  if (it == objects_.end()) {
    return Status::error(ErrorCode::artifact, "cas", "lease unknown artifact");
  }
  it->second.leases++;
  return {};
}

Result<void> ArtifactStore::release(const ArtifactKey& key) {
  auto it = objects_.find(key);
  if (it == objects_.end() || it->second.leases == 0) {
    return Status::error(ErrorCode::artifact, "cas", "release invalid artifact lease");
  }
  it->second.leases--;
  return {};
}

bool ArtifactStore::contains(const ArtifactKey& key) const {
  return objects_.contains(key);
}

}  // namespace forge
