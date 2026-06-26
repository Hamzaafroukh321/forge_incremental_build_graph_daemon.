#include "forge/artifact.hpp"

namespace forge {

void ArtifactWriter::append(const Byte* data, std::size_t size) {
  bytes_.insert(bytes_.end(), data, data + size);
}

Result<ArtifactObject> ArtifactWriter::finish() {
  ArtifactObject object;
  object.bytes = std::move(bytes_);
  object.key = ArtifactKey{digest_bytes("artifact", object.bytes), static_cast<std::uint64_t>(object.bytes.size())};
  object.durable = true;
  return object;
}

}  // namespace forge
