#include "forge/workspace.hpp"

namespace forge {

FileEventCoalescer::FileEventCoalescer(std::uint64_t debounce_ticks) : debounce_ticks_(debounce_ticks) {}

Result<CoalescedEvent> FileEventCoalescer::push(const NormalizedPath& path, FileEventKind kind, std::uint64_t now) {
  auto& entry = pending_[path.relative];
  entry.path = path;
  entry.merged_kind |= static_cast<std::uint32_t>(kind);
  entry.token = entry.token.next();
  entry.due_time = now + debounce_ticks_;
  return entry;
}

std::vector<CoalescedEvent> FileEventCoalescer::due(std::uint64_t now) const {
  std::vector<CoalescedEvent> ready;
  for (const auto& [path, event] : pending_) {
    (void)path;
    if (event.due_time <= now) {
      ready.push_back(event);
    }
  }
  return ready;
}

Result<void> FileEventCoalescer::complete(const CoalescedEvent& event, const InputFingerprint& fingerprint) {
  auto it = pending_.find(event.path.relative);
  if (it == pending_.end() || it->second.token != event.token) {
    return Status::error(ErrorCode::input_unstable, "coalescer", "stale fingerprint completion");
  }
  auto stored = fingerprint;
  auto old = fingerprints_.find(event.path.relative);
  stored.epoch = old == fingerprints_.end() ? Generation{1} : old->second.epoch.next();
  fingerprints_[event.path.relative] = stored;
  pending_.erase(it);
  return {};
}

std::optional<InputFingerprint> FileEventCoalescer::current(std::string_view relative) const {
  auto it = fingerprints_.find(std::string(relative));
  if (it == fingerprints_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace forge
