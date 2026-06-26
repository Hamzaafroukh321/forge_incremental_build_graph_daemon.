#pragma once

#include "forge/event.hpp"
#include "forge/executor.hpp"
#include "forge/state.hpp"

#include <filesystem>
#include <functional>
#include <queue>

namespace forge {

struct NormalizedPath {
  std::string relative;
};

class PathPolicy {
 public:
  explicit PathPolicy(std::filesystem::path root);
  [[nodiscard]] Result<NormalizedPath> normalize(std::string_view path) const;

 private:
  std::filesystem::path root_;
};

enum class FileEventKind : std::uint8_t { create = 1, modify = 2, remove = 4, rename = 8 };

struct CoalescedEvent {
  NormalizedPath path;
  std::uint32_t merged_kind{0};
  Generation token{};
  std::uint64_t due_time{0};
};

class VirtualClock {
 public:
  [[nodiscard]] std::uint64_t now() const noexcept { return now_; }
  void advance(std::uint64_t delta) noexcept { now_ += delta; }

 private:
  std::uint64_t now_{0};
};

class VirtualFileSystem {
 public:
  void write(std::string path, Bytes bytes);
  void erase(std::string path);
  [[nodiscard]] Result<Bytes> read(std::string_view path) const;
  [[nodiscard]] bool exists(std::string_view path) const;

 private:
  std::map<std::string, Bytes> files_;
};

struct InputFingerprint {
  NormalizedPath path;
  Digest256 digest;
  std::uint64_t size{0};
  Generation epoch{};
};

struct WatchEvent {
  NormalizedPath path;
  FileEventKind kind{FileEventKind::modify};
};

class InotifyWatcher {
 public:
  explicit InotifyWatcher(std::filesystem::path root);
  ~InotifyWatcher();

  InotifyWatcher(const InotifyWatcher&) = delete;
  InotifyWatcher& operator=(const InotifyWatcher&) = delete;

  [[nodiscard]] Result<void> start();
  [[nodiscard]] Result<std::vector<WatchEvent>> poll_events(int timeout_ms);
  [[nodiscard]] Result<void> stop();

 private:
  std::filesystem::path root_;
  int fd_{-1};
  int watch_fd_{-1};
};

class FileEventCoalescer {
 public:
  explicit FileEventCoalescer(std::uint64_t debounce_ticks = 5);
  [[nodiscard]] Result<CoalescedEvent> push(const NormalizedPath& path, FileEventKind kind, std::uint64_t now);
  [[nodiscard]] std::vector<CoalescedEvent> due(std::uint64_t now) const;
  [[nodiscard]] Result<void> complete(const CoalescedEvent& event, const InputFingerprint& fingerprint);
  [[nodiscard]] std::optional<InputFingerprint> current(std::string_view relative) const;

 private:
  std::uint64_t debounce_ticks_;
  std::map<std::string, CoalescedEvent> pending_;
  std::map<std::string, InputFingerprint> fingerprints_;
};

struct BuildOutcome {
  RequestId request;
  std::vector<BuildEvent> events;
  std::vector<ArtifactKey> artifacts;
};

class WorkspaceCore {
 public:
  explicit WorkspaceCore(std::filesystem::path root = ".");

  [[nodiscard]] Result<NodeId> put_node(std::string name, ActionSpec action);
  [[nodiscard]] Result<EdgeId> put_edge(NodeId from, NodeId to, EdgeKind kind = EdgeKind::declared);
  [[nodiscard]] Result<void> file_event(std::string_view path, FileEventKind kind);
  [[nodiscard]] Result<BuildOutcome> build(const std::vector<NodeId>& targets, Executor& executor);
  [[nodiscard]] const BuildGraph& graph() const noexcept { return graph_; }
  [[nodiscard]] ArtifactStore& artifacts() noexcept { return artifacts_; }
  [[nodiscard]] EventHub& events() noexcept { return event_hub_; }
  [[nodiscard]] VirtualClock& clock() noexcept { return clock_; }
  [[nodiscard]] VirtualFileSystem& fs() noexcept { return fs_; }

 private:
  [[nodiscard]] Result<Digest256> fingerprint_path(const NormalizedPath& path);
  [[nodiscard]] Result<std::vector<NodeId>> plan_order(const std::vector<NodeId>& targets) const;

  PathPolicy path_policy_;
  VirtualClock clock_;
  VirtualFileSystem fs_;
  FileEventCoalescer coalescer_;
  BuildGraph graph_;
  ArtifactStore artifacts_;
  EventHub event_hub_;
  std::uint64_t next_request_{1};
  std::uint64_t next_job_{1};
};

}  // namespace forge
