#include "forge/event.hpp"

namespace forge {

Result<void> EventHub::open_stream(std::uint32_t stream_id, std::uint64_t credit) {
  if (stream_id == 0 || streams_.contains(stream_id)) {
    return Status::error(ErrorCode::protocol, "event-hub", "invalid event stream");
  }
  streams_[stream_id] = Stream{credit, {}, false};
  return {};
}

Result<void> EventHub::add_credit(std::uint32_t stream_id, std::uint64_t credit) {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    return Status::error(ErrorCode::protocol, "event-hub", "unknown event stream");
  }
  auto added = checked_add(it->second.credit, credit, "event-hub");
  if (!added.ok()) {
    return added.status();
  }
  it->second.credit = added.value();
  return {};
}

Result<void> EventHub::publish(std::uint32_t stream_id, BuildEvent event) {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    return Status::error(ErrorCode::protocol, "event-hub", "unknown event stream");
  }
  Stream& stream = it->second;
  if (event.bytes <= stream.credit) {
    stream.credit -= event.bytes;
    stream.queued.push_back(std::move(event));
    return {};
  }
  if (event.terminal) {
    stream.queued.push_back(std::move(event));
    return {};
  }
  if (!stream.gap_emitted) {
    stream.gap_emitted = true;
    stream.queued.push_back(BuildEvent{EventKind::gap, "progress omitted by credit backpressure", false, 0});
  }
  return Status::error(ErrorCode::backpressure, "event-hub", "progress event dropped");
}

std::vector<BuildEvent> EventHub::drain(std::uint32_t stream_id) {
  std::vector<BuildEvent> out;
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    return out;
  }
  while (!it->second.queued.empty()) {
    out.push_back(std::move(it->second.queued.front()));
    it->second.queued.pop_front();
  }
  return out;
}

}  // namespace forge
