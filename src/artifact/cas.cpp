#include "forge/artifact.hpp"

#include <fstream>

namespace forge {

ArtifactStore::ArtifactStore(std::filesystem::path root) : root_(std::move(root)) {}

Result<ArtifactKey> ArtifactStore::publish(ArtifactWriter writer) {
  auto object = writer.finish();
  if (!object.ok()) {
    return object.status();
  }
  const ArtifactKey key = object.value().key;
  if (digest_bytes("artifact", object.value().bytes) != key.digest) {
    return Status::error(ErrorCode::artifact, "cas", "artifact digest mismatch");
  }
  if (root_) {
    std::error_code ec;
    std::filesystem::create_directories(*root_, ec);
    if (ec) {
      return Status::error(ErrorCode::io, "cas", "unable to create artifact store");
    }
    const auto temp = temp_path(key);
    const auto final = object_path(key);
    {
      std::ofstream output(temp, std::ios::binary | std::ios::trunc);
      if (!output) {
        return Status::error(ErrorCode::io, "cas", "unable to open artifact temp file");
      }
      const auto& bytes = object.value().bytes;
      output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      output.flush();
      if (!output) {
        return Status::error(ErrorCode::io, "cas", "unable to write artifact temp file");
      }
    }
    std::filesystem::rename(temp, final, ec);
    if (ec) {
      std::filesystem::remove(temp);
      return Status::error(ErrorCode::io, "cas", "unable to publish artifact file");
    }
  }
  objects_[key] = std::move(object.value());
  return key;
}

Result<Bytes> ArtifactStore::read(const ArtifactKey& key) const {
  auto it = objects_.find(key);
  if (it != objects_.end() && it->second.durable) {
    return it->second.bytes;
  }
  if (!root_) {
    return Status::error(ErrorCode::artifact, "cas", "artifact not found");
  }
  std::ifstream input(object_path(key), std::ios::binary);
  if (!input) {
    return Status::error(ErrorCode::artifact, "cas", "artifact not found");
  }
  Bytes bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if (bytes.size() != key.size || digest_bytes("artifact", bytes) != key.digest) {
    return Status::error(ErrorCode::artifact, "cas", "artifact digest mismatch");
  }
  return bytes;
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
  if (objects_.contains(key)) {
    return true;
  }
  return root_.has_value() && std::filesystem::exists(object_path(key));
}

std::filesystem::path ArtifactStore::object_path(const ArtifactKey& key) const {
  return *root_ / (key.digest.hex() + "." + std::to_string(key.size) + ".cas");
}

std::filesystem::path ArtifactStore::temp_path(const ArtifactKey& key) const {
  return *root_ / (key.digest.hex() + "." + std::to_string(key.size) + ".tmp");
}

}  // namespace forge
