#include "forge/fipc.hpp"

namespace forge {

FipcSession::FipcSession(Limits limits) : limits_(limits), codec_(limits) {}

Result<std::vector<SessionEvent>> FipcSession::feed(const Byte* data, std::size_t size) {
  std::vector<SessionEvent> events;
  while (true) {
    auto decoded = codec_.feed(data, size);
    data = nullptr;
    size = 0;
    if (decoded.state == DecodeState::need_more) {
      return events;
    }
    if (decoded.state == DecodeState::closed) {
      state_ = SessionState::closed;
      return decoded.diagnostic.ok() ? Result<std::vector<SessionEvent>>(events) : Result<std::vector<SessionEvent>>(decoded.diagnostic);
    }
    auto applied = apply_frame(*decoded.frame, events);
    if (!applied.ok()) {
      state_ = SessionState::closed;
      return applied.status();
    }
    if (codec_.buffered() == 0) {
      return events;
    }
  }
}

Result<void> FipcSession::send_credit(std::uint32_t stream_id, std::uint64_t bytes) {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    return Status::error(ErrorCode::protocol, "fipc-session", "credit for unknown stream");
  }
  auto added = checked_add(it->second.credit, bytes, "fipc-session");
  if (!added.ok()) {
    return added.status();
  }
  it->second.credit = added.value();
  return {};
}

Result<void> FipcSession::reserve_send(std::uint32_t stream_id, std::uint64_t bytes, bool terminal) {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    return Status::error(ErrorCode::protocol, "fipc-session", "send on unknown stream");
  }
  if (it->second.credit < bytes) {
    if (terminal) {
      it->second.terminal_pending = true;
      return {};
    }
    return Status::error(ErrorCode::backpressure, "fipc-session", "insufficient stream credit");
  }
  it->second.credit -= bytes;
  return {};
}

Result<void> FipcSession::apply_frame(const FipcFrame& frame, std::vector<SessionEvent>& events) {
  if (state_ == SessionState::accepted) {
    if (frame.type != FrameType::hello || frame.stream_id != 0 || frame.sequence != 0) {
      return Status::error(ErrorCode::protocol, "fipc-session", "expected initial HELLO");
    }
    state_ = SessionState::negotiated;
    events.push_back({frame.type, frame.stream_id, frame.sequence, frame.request_token});
    return {};
  }
  if (state_ == SessionState::negotiated && frame.type == FrameType::open_stream) {
    auto existing = streams_.find(frame.stream_id);
    const auto retired = retired_stream_generations_.find(frame.stream_id);
    if (frame.stream_id == 0 ||
        (existing != streams_.end() && frame.stream_generation <= existing->second.generation) ||
        (retired != retired_stream_generations_.end() && frame.stream_generation <= retired->second)) {
      return Status::error(ErrorCode::protocol, "fipc-session", "illegal stream reuse");
    }
    if (streams_.size() >= limits_.max_streams) {
      return Status::error(ErrorCode::resource_limit, "fipc-session", "stream cap reached");
    }
    streams_[frame.stream_id] = StreamState{frame.stream_generation, 0, 0, false};
    state_ = SessionState::active;
    events.push_back({frame.type, frame.stream_id, frame.sequence, frame.request_token});
    return {};
  }
  if (state_ != SessionState::active && state_ != SessionState::negotiated) {
    return Status::error(ErrorCode::protocol, "fipc-session", "frame after close");
  }
  auto stream = streams_.find(frame.stream_id);
  if (frame.stream_id != 0 && stream == streams_.end()) {
    return Status::error(ErrorCode::protocol, "fipc-session", "unknown stream");
  }
  if (stream != streams_.end()) {
    if (stream->second.generation != frame.stream_generation || stream->second.next_sequence != frame.sequence) {
      return Status::error(ErrorCode::protocol, "fipc-session", "nonmonotonic stream sequence or generation");
    }
    stream->second.next_sequence++;
  }
  if (frame.type == FrameType::credit) {
    auto credit = read_u64_le(frame.payload, 0);
    if (!credit.ok()) {
      return credit.status();
    }
    auto credited = send_credit(frame.stream_id, credit.value());
    if (!credited.ok()) {
      return credited.status();
    }
  }
  if (frame.type == FrameType::build_request) {
    auto prior = idempotency_.find(frame.request_token);
    if (prior != idempotency_.end() && prior->second != frame.payload) {
      return Status::error(ErrorCode::protocol, "fipc-session", "IDEMPOTENCY_CONFLICT");
    }
    idempotency_[frame.request_token] = frame.payload;
  }
  if (frame.type == FrameType::close_stream && stream != streams_.end()) {
    retired_stream_generations_[frame.stream_id] = stream->second.generation;
    streams_.erase(stream);
    state_ = streams_.empty() ? SessionState::negotiated : SessionState::active;
  }
  events.push_back({frame.type, frame.stream_id, frame.sequence, frame.request_token});
  return {};
}

}  // namespace forge
