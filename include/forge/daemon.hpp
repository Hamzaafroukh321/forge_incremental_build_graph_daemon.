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

class UnixSocketDaemon {
 public:
  UnixSocketDaemon(std::filesystem::path socket_path, Daemon& daemon);
  ~UnixSocketDaemon();

  UnixSocketDaemon(const UnixSocketDaemon&) = delete;
  UnixSocketDaemon& operator=(const UnixSocketDaemon&) = delete;

  [[nodiscard]] Result<void> start();
  [[nodiscard]] Result<void> serve_once();
  [[nodiscard]] Result<void> stop();

 private:
  std::filesystem::path socket_path_;
  Daemon& daemon_;
  int listen_fd_{-1};
};

class UnixSocketClient {
 public:
  explicit UnixSocketClient(std::filesystem::path socket_path);

  [[nodiscard]] Result<std::string> status();

 private:
  std::filesystem::path socket_path_;
};

}  // namespace forge
