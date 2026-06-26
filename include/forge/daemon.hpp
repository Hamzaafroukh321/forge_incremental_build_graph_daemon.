#pragma once

#include "forge/client.hpp"

namespace forge {

enum class DaemonState { initialized, running, stopping, stopped };

class Daemon {
 public:
  explicit Daemon(std::filesystem::path root = ".") : workspace_(std::move(root)) {}
  [[nodiscard]] Result<void> start();
  [[nodiscard]] Result<void> stop();
  [[nodiscard]] DaemonState state() const noexcept { return state_; }
  [[nodiscard]] WorkspaceCore& workspace() noexcept { return workspace_; }

 private:
  DaemonState state_{DaemonState::initialized};
  WorkspaceCore workspace_;
};

}  // namespace forge
