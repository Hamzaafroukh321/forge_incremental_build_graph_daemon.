#pragma once

#include <cstdint>
#include <compare>
#include <string>

namespace forge {

template <class Tag>
struct Id {
  std::uint64_t value{0};
  friend bool operator==(Id, Id) = default;
  friend auto operator<=>(Id, Id) = default;
  [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
};

struct NodeTag;
struct EdgeTag;
struct RequestTag;
struct JobTag;
struct LeaseTag;
struct StreamTag;
struct ArtifactTag;
struct WorkspaceTag;

using NodeId = Id<NodeTag>;
using EdgeId = Id<EdgeTag>;
using RequestId = Id<RequestTag>;
using JobId = Id<JobTag>;
using LeaseId = Id<LeaseTag>;
using StreamId = Id<StreamTag>;
using ArtifactId = Id<ArtifactTag>;
using WorkspaceId = Id<WorkspaceTag>;

struct Generation {
  std::uint64_t value{1};
  friend bool operator==(Generation, Generation) = default;
  friend auto operator<=>(Generation, Generation) = default;
  [[nodiscard]] Generation next() const noexcept { return Generation{value + 1}; }
};

template <class Tag>
struct Handle {
  Id<Tag> id{};
  Generation generation{};
  friend bool operator==(Handle, Handle) = default;
};

template <class Tag>
std::string id_string(Id<Tag> id) {
  return std::to_string(id.value);
}

}  // namespace forge
