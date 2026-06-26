#include "forge/error.hpp"

#include <iomanip>
#include <sstream>

namespace forge {

std::string_view error_code_name(ErrorCode code) noexcept {
  switch (code) {
    case ErrorCode::ok: return "Ok";
    case ErrorCode::usage: return "Usage";
    case ErrorCode::protocol: return "Protocol";
    case ErrorCode::format: return "Format";
    case ErrorCode::graph_conflict: return "GraphConflict";
    case ErrorCode::cycle: return "Cycle";
    case ErrorCode::path: return "Path";
    case ErrorCode::input_unstable: return "InputUnstable";
    case ErrorCode::executor: return "Executor";
    case ErrorCode::artifact: return "Artifact";
    case ErrorCode::state_recovery: return "StateRecovery";
    case ErrorCode::resource_limit: return "ResourceLimit";
    case ErrorCode::backpressure: return "Backpressure";
    case ErrorCode::cancelled: return "Cancelled";
    case ErrorCode::io: return "Io";
    case ErrorCode::internal_invariant: return "InternalInvariant";
  }
  return "Unknown";
}

void append_u16_le(Bytes& out, std::uint16_t value) {
  out.push_back(static_cast<Byte>(value & 0xffU));
  out.push_back(static_cast<Byte>((value >> 8U) & 0xffU));
}

void append_u32_le(Bytes& out, std::uint32_t value) {
  for (int shift = 0; shift != 32; shift += 8) {
    out.push_back(static_cast<Byte>((value >> static_cast<unsigned>(shift)) & 0xffU));
  }
}

void append_u64_le(Bytes& out, std::uint64_t value) {
  for (int shift = 0; shift != 64; shift += 8) {
    out.push_back(static_cast<Byte>((value >> static_cast<unsigned>(shift)) & 0xffU));
  }
}

Result<std::uint16_t> read_u16_le(const Bytes& bytes, std::size_t offset) {
  if (offset > bytes.size() || bytes.size() - offset < 2) {
    auto status = Status::error(ErrorCode::format, "codec", "truncated u16");
    status.offset = offset;
    return status;
  }
  return static_cast<std::uint16_t>(bytes[offset] | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U));
}

Result<std::uint32_t> read_u32_le(const Bytes& bytes, std::size_t offset) {
  if (offset > bytes.size() || bytes.size() - offset < 4) {
    auto status = Status::error(ErrorCode::format, "codec", "truncated u32");
    status.offset = offset;
    return status;
  }
  std::uint32_t value = 0;
  for (std::size_t i = 0; i != 4; ++i) {
    value |= static_cast<std::uint32_t>(bytes[offset + i]) << (i * 8U);
  }
  return value;
}

Result<std::uint64_t> read_u64_le(const Bytes& bytes, std::size_t offset) {
  if (offset > bytes.size() || bytes.size() - offset < 8) {
    auto status = Status::error(ErrorCode::format, "codec", "truncated u64");
    status.offset = offset;
    return status;
  }
  std::uint64_t value = 0;
  for (std::size_t i = 0; i != 8; ++i) {
    value |= static_cast<std::uint64_t>(bytes[offset + i]) << (i * 8U);
  }
  return value;
}

std::uint32_t crc32c(const Byte* data, std::size_t size) noexcept {
  std::uint32_t crc = 0xffffffffU;
  for (std::size_t i = 0; i != size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit != 8; ++bit) {
      const std::uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0x82f63b78U & mask);
    }
  }
  return ~crc;
}

std::uint32_t crc32c(const Bytes& bytes) noexcept {
  return crc32c(bytes.data(), bytes.size());
}

std::string Digest256::hex() const {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto word : words) {
    out << std::setw(16) << word;
  }
  return out.str();
}

Digest256 digest_bytes(std::string_view domain, const Byte* data, std::size_t size) {
  Digest256 digest{{1469598103934665603ULL, 1099511628211ULL, 7809847782465536322ULL, 1609587929392839161ULL}};
  auto mix = [&](Byte byte) {
    for (std::uint64_t& word : digest.words) {
      word ^= byte;
      word *= 1099511628211ULL;
      word ^= (word >> 32U);
    }
  };
  for (const char ch : domain) {
    mix(static_cast<Byte>(ch));
  }
  mix(0xffU);
  for (std::size_t i = 0; i != size; ++i) {
    mix(data[i]);
  }
  return digest;
}

Digest256 digest_bytes(std::string_view domain, const Bytes& bytes) {
  return digest_bytes(domain, bytes.data(), bytes.size());
}

}  // namespace forge
