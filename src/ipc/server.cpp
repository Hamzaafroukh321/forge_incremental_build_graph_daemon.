#include "forge/daemon.hpp"
#include "forge/fipc.hpp"

#include <array>
#include <sstream>

#ifndef _WIN32
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace forge {
namespace {

std::string daemon_state_name(DaemonState state) {
  switch (state) {
    case DaemonState::initialized: return "initialized";
    case DaemonState::running: return "running";
    case DaemonState::stopping: return "stopping";
    case DaemonState::stopped: return "stopped";
  }
  return "unknown";
}

#ifndef _WIN32

Status socket_error(std::string component, std::string message) {
  message += ": ";
  message += std::strerror(errno);
  return Status::error(ErrorCode::io, std::move(component), std::move(message));
}

Result<void> write_all(int fd, const Bytes& bytes) {
  std::size_t written = 0;
  while (written < bytes.size()) {
    const auto result = ::send(fd,
                               bytes.data() + static_cast<std::ptrdiff_t>(written),
                               bytes.size() - written,
                               MSG_NOSIGNAL);
    if (result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return socket_error("unix-socket-daemon", "unable to write response");
    }
    written += static_cast<std::size_t>(result);
  }
  return {};
}

Result<sockaddr_un> socket_address(const std::filesystem::path& path) {
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  const auto native = path.string();
  if (native.size() >= sizeof(addr.sun_path)) {
    return Status::error(ErrorCode::path, "unix-socket-daemon", "socket path is too long");
  }
  std::memcpy(addr.sun_path, native.c_str(), native.size() + 1);
  return addr;
}

#endif

}  // namespace

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

UnixSocketDaemon::UnixSocketDaemon(std::filesystem::path socket_path, Daemon& daemon)
    : socket_path_(std::move(socket_path)), daemon_(daemon) {}

UnixSocketDaemon::~UnixSocketDaemon() {
  (void)stop();
}

Result<void> UnixSocketDaemon::start() {
#ifdef _WIN32
  return Status::error(ErrorCode::io, "unix-socket-daemon", "Unix-domain sockets are not supported on Windows");
#else
  if (listen_fd_ >= 0) {
    return Status::error(ErrorCode::internal_invariant, "unix-socket-daemon", "socket daemon already started");
  }
  auto started = daemon_.start();
  if (!started.ok()) {
    return started.status();
  }
  const auto parent = socket_path_.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      return Status::error(ErrorCode::io, "unix-socket-daemon", "unable to create socket directory");
    }
  }
  auto addr = socket_address(socket_path_);
  if (!addr.ok()) {
    return addr.status();
  }
  listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    return socket_error("unix-socket-daemon", "unable to create socket");
  }
  ::unlink(socket_path_.string().c_str());
  if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr.value()), sizeof(addr.value())) != 0) {
    const auto status = socket_error("unix-socket-daemon", "unable to bind socket");
    (void)stop();
    return status;
  }
  if (::listen(listen_fd_, 16) != 0) {
    const auto status = socket_error("unix-socket-daemon", "unable to listen on socket");
    (void)stop();
    return status;
  }
  return {};
#endif
}

Result<void> UnixSocketDaemon::serve_once() {
#ifdef _WIN32
  return Status::error(ErrorCode::io, "unix-socket-daemon", "Unix-domain sockets are not supported on Windows");
#else
  if (listen_fd_ < 0) {
    return Status::error(ErrorCode::internal_invariant, "unix-socket-daemon", "socket daemon is not started");
  }
  const int client_fd = ::accept(listen_fd_, nullptr, nullptr);
  if (client_fd < 0) {
    return socket_error("unix-socket-daemon", "unable to accept connection");
  }
  FipcSession session;
  FipcCodec encoder;
  std::array<Byte, 4096> buffer{};
  while (true) {
    const auto received = ::recv(client_fd, buffer.data(), buffer.size(), 0);
    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }
      const auto status = socket_error("unix-socket-daemon", "unable to read request");
      ::close(client_fd);
      return status;
    }
    if (received == 0) {
      ::close(client_fd);
      return Status::error(ErrorCode::protocol, "unix-socket-daemon", "client closed before request");
    }
    auto events = session.feed(buffer.data(), static_cast<std::size_t>(received));
    if (!events.ok()) {
      ::close(client_fd);
      return events.status();
    }
    for (const auto& event : events.value()) {
      if (event.type == FrameType::hello) {
        auto welcome = encoder.encode(FipcFrame{FrameType::welcome, 0, 0, 0, 0, event.request_token, {}});
        auto written = write_all(client_fd, welcome);
        if (!written.ok()) {
          ::close(client_fd);
          return written.status();
        }
      }
      if (event.type == FrameType::build_request) {
        std::ostringstream out;
        out << "state=" << daemon_state_name(daemon_.state()) << " generation=" << daemon_.workspace().graph().generation().value << "\n";
        const auto text = out.str();
        Bytes payload(text.begin(), text.end());
        auto response = encoder.encode(FipcFrame{FrameType::status_event, 0, event.stream_id, 1, 0, event.request_token, payload});
        auto written = write_all(client_fd, response);
        ::close(client_fd);
        return written;
      }
    }
  }
#endif
}

Result<void> UnixSocketDaemon::stop() {
#ifdef _WIN32
  return daemon_.stop();
#else
  if (listen_fd_ >= 0) {
    ::close(listen_fd_);
    listen_fd_ = -1;
  }
  ::unlink(socket_path_.string().c_str());
  return daemon_.stop();
#endif
}

}  // namespace forge
