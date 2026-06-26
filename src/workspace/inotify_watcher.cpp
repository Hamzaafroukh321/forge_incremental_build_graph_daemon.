#include "forge/workspace.hpp"

#include <array>

#ifndef _WIN32
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace forge {
namespace {

#ifndef _WIN32

Status inotify_error(std::string message) {
  message += ": ";
  message += std::strerror(errno);
  return Status::error(ErrorCode::io, "inotify-watcher", std::move(message));
}

std::optional<FileEventKind> map_kind(std::uint32_t mask) {
  if ((mask & (IN_DELETE | IN_MOVED_FROM | IN_DELETE_SELF | IN_MOVE_SELF)) != 0U) {
    return FileEventKind::remove;
  }
  if ((mask & (IN_CREATE | IN_MOVED_TO)) != 0U) {
    return FileEventKind::create;
  }
  if ((mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB)) != 0U) {
    return FileEventKind::modify;
  }
  return std::nullopt;
}

#endif

}  // namespace

InotifyWatcher::InotifyWatcher(std::filesystem::path root) : root_(std::move(root)) {}

InotifyWatcher::~InotifyWatcher() {
  (void)stop();
}

Result<void> InotifyWatcher::start() {
#ifdef _WIN32
  return Status::error(ErrorCode::io, "inotify-watcher", "inotify is not supported on Windows");
#else
  if (fd_ >= 0) {
    return Status::error(ErrorCode::internal_invariant, "inotify-watcher", "watcher already started");
  }
  std::error_code ec;
  std::filesystem::create_directories(root_, ec);
  if (ec) {
    return Status::error(ErrorCode::io, "inotify-watcher", "unable to create watch root");
  }
  fd_ = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (fd_ < 0) {
    return inotify_error("unable to create inotify descriptor");
  }
  constexpr std::uint32_t kMask = IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO |
                                  IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
  watch_fd_ = ::inotify_add_watch(fd_, root_.string().c_str(), kMask);
  if (watch_fd_ < 0) {
    const auto status = inotify_error("unable to add inotify watch");
    (void)stop();
    return status;
  }
  return {};
#endif
}

Result<std::vector<WatchEvent>> InotifyWatcher::poll_events(int timeout_ms) {
#ifdef _WIN32
  (void)timeout_ms;
  return Status::error(ErrorCode::io, "inotify-watcher", "inotify is not supported on Windows");
#else
  if (fd_ < 0) {
    return Status::error(ErrorCode::internal_invariant, "inotify-watcher", "watcher is not started");
  }
  pollfd descriptor{fd_, POLLIN, 0};
  while (true) {
    const int ready = ::poll(&descriptor, 1, timeout_ms);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      return inotify_error("unable to poll inotify descriptor");
    }
    if (ready == 0) {
      return std::vector<WatchEvent>{};
    }
    break;
  }

  std::vector<WatchEvent> events;
  std::array<Byte, 4096> buffer{};
  while (true) {
    const auto bytes = ::read(fd_, buffer.data(), buffer.size());
    if (bytes < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return events;
      }
      return inotify_error("unable to read inotify events");
    }
    if (bytes == 0) {
      return events;
    }
    std::size_t offset = 0;
    while (offset < static_cast<std::size_t>(bytes)) {
      const auto* event = reinterpret_cast<const inotify_event*>(buffer.data() + static_cast<std::ptrdiff_t>(offset));
      const std::size_t record_size = sizeof(inotify_event) + event->len;
      offset += record_size;
      if ((event->mask & IN_IGNORED) != 0U || event->len == 0) {
        continue;
      }
      auto kind = map_kind(event->mask);
      if (!kind) {
        continue;
      }
      std::string relative(event->name);
      if (relative.empty()) {
        continue;
      }
      events.push_back(WatchEvent{NormalizedPath{std::move(relative)}, *kind});
    }
  }
#endif
}

Result<void> InotifyWatcher::stop() {
#ifdef _WIN32
  return {};
#else
  if (fd_ >= 0 && watch_fd_ >= 0) {
    ::inotify_rm_watch(fd_, watch_fd_);
    watch_fd_ = -1;
  }
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  return {};
#endif
}

}  // namespace forge
