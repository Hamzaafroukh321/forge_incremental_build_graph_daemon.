#pragma once

#include "forge/error.hpp"
#include "forge/ids.hpp"

#include <map>
#include <optional>
#include <set>

namespace forge {

enum class FrameType : std::uint16_t {
  hello = 0x0001,
  welcome = 0x0002,
  open_stream = 0x0010,
  credit = 0x0011,
  close_stream = 0x0012,
  graph_txn = 0x0020,
  build_request = 0x0030,
  cancel_request = 0x0031,
  status_event = 0x0040,
  worker_result = 0x0050,
  state_txn_begin = 0x1000,
  graph_delta = 0x1010,
  input_delta = 0x1011,
  job_delta = 0x1012,
  state_txn_commit = 0x10F0,
  checkpoint = 0x10F1
};

struct FipcFrame {
  FrameType type{FrameType::hello};
  std::uint16_t flags{0};
  std::uint32_t stream_id{0};
  std::uint32_t stream_generation{0};
  std::uint64_t sequence{0};
  std::uint64_t request_token{0};
  Bytes payload;
};

enum class DecodeState { need_more, frame, closed };

struct DecodeResult {
  DecodeState state{DecodeState::need_more};
  std::optional<FipcFrame> frame;
  Status diagnostic;
};

class FipcCodec {
 public:
  explicit FipcCodec(Limits limits = {});

  [[nodiscard]] Bytes encode(const FipcFrame& frame) const;
  [[nodiscard]] DecodeResult feed(const Byte* data, std::size_t size);
  [[nodiscard]] DecodeResult eof();
  [[nodiscard]] std::size_t buffered() const noexcept { return buffer_.size(); }

 private:
  Limits limits_;
  Bytes buffer_;
  bool closed_{false};
};

enum class SessionState { accepted, negotiated, active, closed };

struct SessionEvent {
  FrameType type;
  std::uint32_t stream_id;
  std::uint64_t sequence;
  std::uint64_t request_token;
};

class FipcSession {
 public:
  explicit FipcSession(Limits limits = {});

  [[nodiscard]] Result<std::vector<SessionEvent>> feed(const Byte* data, std::size_t size);
  [[nodiscard]] Result<void> send_credit(std::uint32_t stream_id, std::uint64_t bytes);
  [[nodiscard]] Result<void> reserve_send(std::uint32_t stream_id, std::uint64_t bytes, bool terminal);
  [[nodiscard]] Bytes encode(const FipcFrame& frame) const { return codec_.encode(frame); }
  [[nodiscard]] SessionState state() const noexcept { return state_; }

 private:
  struct StreamState {
    std::uint32_t generation{0};
    std::uint64_t next_sequence{0};
    std::uint64_t credit{0};
    bool terminal_pending{false};
  };

  Result<void> apply_frame(const FipcFrame& frame, std::vector<SessionEvent>& events);

  Limits limits_;
  FipcCodec codec_;
  SessionState state_{SessionState::accepted};
  std::map<std::uint32_t, StreamState> streams_;
  std::map<std::uint64_t, Bytes> idempotency_;
};

}  // namespace forge
