#pragma once

#include "forge/error.hpp"

#include <deque>
#include <map>

namespace forge {

enum class EventKind { progress, gap, queued, running, succeeded, failed, cancelled };

struct BuildEvent {
  EventKind kind{EventKind::progress};
  std::string message;
  bool terminal{false};
  std::uint64_t bytes{0};
};

class EventHub {
 public:
  [[nodiscard]] Result<void> open_stream(std::uint32_t stream_id, std::uint64_t credit);
  [[nodiscard]] Result<void> add_credit(std::uint32_t stream_id, std::uint64_t credit);
  [[nodiscard]] Result<void> publish(std::uint32_t stream_id, BuildEvent event);
  [[nodiscard]] std::vector<BuildEvent> drain(std::uint32_t stream_id);

 private:
  struct Stream {
    std::uint64_t credit{0};
    std::deque<BuildEvent> queued;
    bool gap_emitted{false};
  };
  std::map<std::uint32_t, Stream> streams_;
};

}  // namespace forge
