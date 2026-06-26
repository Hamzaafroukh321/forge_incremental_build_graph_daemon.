#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace forge {

enum class ErrorCode {
  ok,
  usage,
  protocol,
  format,
  graph_conflict,
  cycle,
  path,
  input_unstable,
  executor,
  artifact,
  state_recovery,
  resource_limit,
  backpressure,
  cancelled,
  io,
  internal_invariant
};

struct Status {
  ErrorCode code{ErrorCode::ok};
  std::string component;
  std::string message;
  std::uint64_t generation{0};
  std::uint64_t id{0};
  std::size_t offset{0};

  [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::ok; }
  static Status success() { return {}; }
  static Status error(ErrorCode code_value, std::string component_value, std::string message_value) {
    return Status{code_value, std::move(component_value), std::move(message_value), 0, 0, 0};
  }
};

std::string_view error_code_name(ErrorCode code) noexcept;

template <class T>
class Result {
 public:
  Result(T value) : value_(std::move(value)) {}
  Result(Status status) : status_(std::move(status)) {}

  [[nodiscard]] bool ok() const noexcept { return status_.ok(); }
  [[nodiscard]] const Status& status() const noexcept { return status_; }
  [[nodiscard]] T& value() & { return *value_; }
  [[nodiscard]] const T& value() const& { return *value_; }
  [[nodiscard]] T&& value() && { return std::move(*value_); }

 private:
  std::optional<T> value_;
  Status status_;
};

template <>
class Result<void> {
 public:
  Result() = default;
  Result(Status status) : status_(std::move(status)) {}
  [[nodiscard]] bool ok() const noexcept { return status_.ok(); }
  [[nodiscard]] const Status& status() const noexcept { return status_; }

 private:
  Status status_;
};

struct Limits {
  std::uint32_t max_frame_bytes{16U * 1024U * 1024U};
  std::uint32_t max_payload_bytes{16U * 1024U * 1024U};
  std::uint32_t max_streams{1024};
  std::uint32_t max_nodes{500000};
  std::uint32_t max_edges{2000000};
  std::uint32_t max_discovered_edges{200000};
  std::uint32_t max_event_backlog_bytes{8U * 1024U * 1024U};
  std::uint32_t max_single_allocation{32U * 1024U * 1024U};
};

template <class T>
Result<T> checked_add(T left, T right, std::string component) {
  static_assert(std::numeric_limits<T>::is_integer);
  if (right > 0 && left > std::numeric_limits<T>::max() - right) {
    return Status::error(ErrorCode::resource_limit, std::move(component), "integer addition overflow");
  }
  return static_cast<T>(left + right);
}

template <class T>
Result<T> checked_mul(T left, T right, std::string component) {
  static_assert(std::numeric_limits<T>::is_integer);
  if (left != 0 && right > std::numeric_limits<T>::max() / left) {
    return Status::error(ErrorCode::resource_limit, std::move(component), "integer multiplication overflow");
  }
  return static_cast<T>(left * right);
}

using Byte = std::uint8_t;
using Bytes = std::vector<Byte>;

void append_u16_le(Bytes& out, std::uint16_t value);
void append_u32_le(Bytes& out, std::uint32_t value);
void append_u64_le(Bytes& out, std::uint64_t value);
Result<std::uint16_t> read_u16_le(const Bytes& bytes, std::size_t offset);
Result<std::uint32_t> read_u32_le(const Bytes& bytes, std::size_t offset);
Result<std::uint64_t> read_u64_le(const Bytes& bytes, std::size_t offset);
std::uint32_t crc32c(const Byte* data, std::size_t size) noexcept;
std::uint32_t crc32c(const Bytes& bytes) noexcept;

struct Digest256 {
  std::array<std::uint64_t, 4> words{0, 0, 0, 0};

  friend bool operator==(const Digest256&, const Digest256&) = default;
  [[nodiscard]] std::string hex() const;
};

Digest256 digest_bytes(std::string_view domain, const Byte* data, std::size_t size);
Digest256 digest_bytes(std::string_view domain, const Bytes& bytes);

}  // namespace forge
