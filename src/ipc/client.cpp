#include "forge/client.hpp"
#include "forge/daemon.hpp"
#include "forge/fipc.hpp"

#include <array>

#ifndef _WIN32
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace forge {
namespace {
static_assert(sizeof(Client*) == sizeof(void*));

constexpr std::string_view kStatusPayload = "status";

#ifndef _WIN32

Status socket_error(std::string message) {
  message += ": ";
  message += std::strerror(errno);
  return Status::error(ErrorCode::io, "unix-socket-client", std::move(message));
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
      return socket_error("unable to write request");
    }
    written += static_cast<std::size_t>(result);
  }
  return {};
}

Result<int> connect_socket(const std::filesystem::path& path) {
  const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return socket_error("unable to create socket");
  }
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  const auto native = path.string();
  if (native.size() >= sizeof(addr.sun_path)) {
    ::close(fd);
    return Status::error(ErrorCode::path, "unix-socket-client", "socket path is too long");
  }
  std::memcpy(addr.sun_path, native.c_str(), native.size() + 1);
  if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    const auto status = socket_error("unable to connect");
    ::close(fd);
    return status;
  }
  return fd;
}

#endif
}

UnixSocketClient::UnixSocketClient(std::filesystem::path socket_path) : socket_path_(std::move(socket_path)) {}

Result<std::string> UnixSocketClient::status() {
#ifdef _WIN32
  return Status::error(ErrorCode::io, "unix-socket-client", "Unix-domain sockets are not supported on Windows");
#else
  auto connected = connect_socket(socket_path_);
  if (!connected.ok()) {
    return connected.status();
  }
  const int fd = connected.value();
  FipcCodec codec;
  FipcCodec encoder;
  Bytes request;
  auto append_frame = [&](const FipcFrame& frame) {
    auto encoded = encoder.encode(frame);
    request.insert(request.end(), encoded.begin(), encoded.end());
  };
  append_frame(FipcFrame{FrameType::hello, 0, 0, 0, 0, 1, {}});
  append_frame(FipcFrame{FrameType::open_stream, 0, 1, 1, 0, 1, {}});
  append_frame(FipcFrame{FrameType::build_request,
                         0,
                         1,
                         1,
                         0,
                         2,
                         Bytes(kStatusPayload.begin(), kStatusPayload.end())});
  auto written = write_all(fd, request);
  if (!written.ok()) {
    ::close(fd);
    return written.status();
  }

  std::array<Byte, 4096> buffer{};
  while (true) {
    const auto received = ::recv(fd, buffer.data(), buffer.size(), 0);
    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }
      const auto status = socket_error("unable to read response");
      ::close(fd);
      return status;
    }
    if (received == 0) {
      ::close(fd);
      return Status::error(ErrorCode::protocol, "unix-socket-client", "daemon closed before status response");
    }
    auto decoded = codec.feed(buffer.data(), static_cast<std::size_t>(received));
    while (decoded.state == DecodeState::frame) {
      if (decoded.frame->type == FrameType::status_event) {
        std::string payload(decoded.frame->payload.begin(), decoded.frame->payload.end());
        ::close(fd);
        return payload;
      }
      if (codec.buffered() == 0) {
        break;
      }
      decoded = codec.feed(nullptr, 0);
    }
    if (decoded.state == DecodeState::closed && !decoded.diagnostic.ok()) {
      ::close(fd);
      return decoded.diagnostic;
    }
  }
#endif
}
}  // namespace forge
