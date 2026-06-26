#pragma once

#include "forge/error.hpp"
#include "forge/ids.hpp"

#include <compare>
#include <filesystem>
#include <map>
#include <optional>
#include <set>

namespace forge {

struct ArtifactKey {
  Digest256 digest;
  std::uint64_t size{0};
  friend bool operator==(const ArtifactKey&, const ArtifactKey&) = default;
  friend auto operator<=>(const ArtifactKey& left, const ArtifactKey& right) {
    if (auto cmp = left.digest.hex() <=> right.digest.hex(); cmp != 0) {
      return cmp;
    }
    return left.size <=> right.size;
  }
};

struct ArtifactObject {
  ArtifactKey key;
  Bytes bytes;
  std::uint64_t leases{0};
  bool durable{false};
};

class ArtifactWriter {
 public:
  void append(const Byte* data, std::size_t size);
  [[nodiscard]] Result<ArtifactObject> finish();

 private:
  Bytes bytes_;
};

class ArtifactStore {
 public:
  ArtifactStore() = default;
  explicit ArtifactStore(std::filesystem::path root);

  [[nodiscard]] Result<ArtifactKey> publish(ArtifactWriter writer);
  [[nodiscard]] Result<Bytes> read(const ArtifactKey& key) const;
  [[nodiscard]] Result<void> acquire(const ArtifactKey& key);
  [[nodiscard]] Result<void> release(const ArtifactKey& key);
  [[nodiscard]] std::size_t gc();
  [[nodiscard]] bool contains(const ArtifactKey& key) const;

 private:
  [[nodiscard]] std::filesystem::path object_path(const ArtifactKey& key) const;
  [[nodiscard]] std::filesystem::path temp_path(const ArtifactKey& key) const;

  std::optional<std::filesystem::path> root_;
  std::map<ArtifactKey, ArtifactObject> objects_;
};

}  // namespace forge
