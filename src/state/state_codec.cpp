#include "forge/state.hpp"

namespace forge {
namespace {
constexpr std::size_t kStateHeaderSize = 40;
}

Bytes StateCodec::encode(const StateRecord& record) const {
  Bytes out;
  append_u32_le(out, static_cast<std::uint32_t>(kStateHeaderSize + record.payload.size()));
  append_u16_le(out, static_cast<std::uint16_t>(record.type));
  append_u16_le(out, 0);
  append_u64_le(out, record.txn_id);
  append_u64_le(out, record.base_sequence);
  append_u64_le(out, record.new_sequence);
  append_u32_le(out, 0);
  append_u32_le(out, crc32c(record.payload));
  const std::uint32_t header_crc = crc32c(out.data(), kStateHeaderSize - 8);
  out[32] = static_cast<Byte>(header_crc & 0xffU);
  out[33] = static_cast<Byte>((header_crc >> 8U) & 0xffU);
  out[34] = static_cast<Byte>((header_crc >> 16U) & 0xffU);
  out[35] = static_cast<Byte>((header_crc >> 24U) & 0xffU);
  out.insert(out.end(), record.payload.begin(), record.payload.end());
  while (out.size() % 8 != 0) {
    out.push_back(0);
  }
  return out;
}

Result<StateRecord> StateCodec::decode(const Bytes& bytes, std::size_t* consumed) const {
  if (bytes.size() < kStateHeaderSize) {
    return Status::error(ErrorCode::format, "state-codec", "truncated state record");
  }
  const auto len = read_u32_le(bytes, 0).value();
  if (static_cast<std::size_t>(len) < kStateHeaderSize || static_cast<std::size_t>(len) > bytes.size()) {
    return Status::error(ErrorCode::format, "state-codec", "invalid state record length");
  }
  Bytes header(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(kStateHeaderSize));
  const auto expected_header_crc = read_u32_le(header, 32).value();
  header[32] = header[33] = header[34] = header[35] = 0;
  if (crc32c(header.data(), kStateHeaderSize - 8) != expected_header_crc) {
    return Status::error(ErrorCode::format, "state-codec", "state header crc mismatch");
  }
  StateRecord record;
  record.type = static_cast<StateRecordType>(read_u16_le(bytes, 4).value());
  record.txn_id = read_u64_le(bytes, 8).value();
  record.base_sequence = read_u64_le(bytes, 16).value();
  record.new_sequence = read_u64_le(bytes, 24).value();
  const auto payload_crc = read_u32_le(bytes, 36).value();
  record.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kStateHeaderSize), bytes.begin() + static_cast<std::ptrdiff_t>(len));
  if (crc32c(record.payload) != payload_crc) {
    return Status::error(ErrorCode::format, "state-codec", "state payload crc mismatch");
  }
  *consumed = len;
  while (*consumed < bytes.size() && (*consumed % 8) != 0) {
    ++*consumed;
  }
  return record;
}

}  // namespace forge
