#include "forge/daemon.hpp"

namespace forge {

Result<void> Daemon::start() {
  if (state_ != DaemonState::initialized && state_ != DaemonState::stopped) {
    return Status::error(ErrorCode::internal_invariant, "daemon", "daemon already running");
  }
  state_ = DaemonState::running;
  return {};
}

Result<void> Daemon::stop() {
  if (state_ == DaemonState::stopped) {
    return {};
  }
  state_ = DaemonState::stopping;
  state_ = DaemonState::stopped;
  return {};
}

}  // namespace forge
