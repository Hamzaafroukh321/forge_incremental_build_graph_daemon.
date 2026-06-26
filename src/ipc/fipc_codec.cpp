#include "forge/fipc.hpp"

#include <algorithm>

namespace forge {
namespace {
constexpr std::size_t kFrameHeaderSize = 36;

Status protocol_error(std::string message) {
  return Status::error(ErrorCode::protocol, "fipc", std::move(message));
}
}  // namespace

FipcCodec::FipcCodec(Limits limits) : limits_(limits) {}

Bytes FipcCodec::encode(const FipcFrame& frame) const {
  Bytes out;
  const std::uint32_t frame_len = static_cast<std::uint32_t>(kFrameHeaderSize + frame.payload.size());
  append_u32_le(out, frame_len);
  append_u16_le(out, static_cast<std::uint16_t>(frame.type));
  append_u16_le(out, frame.flags);
  append_u32_le(out, frame.stream_id);
  append_u32_le(out, frame.stream_generation);
  append_u64_le(out, frame.sequence);
  append_u64_le(out, frame.request_token);
  append_u32_le(out, 0);
  append_u32_le(out, crc32c(frame.payload));
  const std::uint32_t header_crc = crc32c(out.data(), kFrameHeaderSize - 8);
  out[28] = static_cast<Byte>(header_crc & 0xffU);
  out[29] = static_cast<Byte>((header_crc >> 8U) & 0xffU);
  out[30] = static_cast<Byte>((header_crc >> 16U) & 0xffU);
  out[31] = static_cast<Byte>((header_crc >> 24U) & 0xffU);
  out.insert(out.end(), frame.payload.begin(), frame.payload.end());
  return out;
}

DecodeResult FipcCodec::feed(const Byte* data, std::size_t size) {
  if (closed_) {
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("session already closed")};
  }
  if (size > limits_.max_single_allocation || buffer_.size() > limits_.max_single_allocation - size) {
    closed_ = true;
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("connection buffer limit exceeded")};
  }
  if (size != 0) {
    buffer_.insert(buffer_.end(), data, data + size);
  }
  if (buffer_.size() < kFrameHeaderSize) {
    return {};
  }
  auto len_result = read_u32_le(buffer_, 0);
  if (!len_result.ok()) {
    return DecodeResult{DecodeState::need_more, std::nullopt, len_result.status()};
  }
  const std::uint32_t frame_len = len_result.value();
  if (frame_len < kFrameHeaderSize || frame_len > limits_.max_frame_bytes) {
    closed_ = true;
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("invalid frame length")};
  }
  if (buffer_.size() < frame_len) {
    return {};
  }
  Bytes header(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize));
  const auto expected_header_crc = read_u32_le(header, 28).value();
  header[28] = 0;
  header[29] = 0;
  header[30] = 0;
  header[31] = 0;
  if (crc32c(header.data(), kFrameHeaderSize - 8) != expected_header_crc) {
    closed_ = true;
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("header crc mismatch")};
  }
  FipcFrame frame;
  frame.type = static_cast<FrameType>(read_u16_le(buffer_, 4).value());
  frame.flags = read_u16_le(buffer_, 6).value();
  frame.stream_id = read_u32_le(buffer_, 8).value();
  frame.stream_generation = read_u32_le(buffer_, 12).value();
  frame.sequence = read_u64_le(buffer_, 16).value();
  frame.request_token = read_u64_le(buffer_, 24).value();
  const auto payload_crc = read_u32_le(buffer_, 32).value();
  frame.payload.assign(buffer_.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize), buffer_.begin() + frame_len);
  if (frame.payload.size() > limits_.max_payload_bytes || crc32c(frame.payload) != payload_crc) {
    closed_ = true;
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("payload crc mismatch")};
  }
  buffer_.erase(buffer_.begin(), buffer_.begin() + frame_len);
  return DecodeResult{DecodeState::frame, std::move(frame), Status::success()};
}

DecodeResult FipcCodec::eof() {
  if (!buffer_.empty()) {
    closed_ = true;
    return DecodeResult{DecodeState::closed, std::nullopt, protocol_error("truncated frame at eof")};
  }
  closed_ = true;
  return DecodeResult{DecodeState::closed, std::nullopt, Status::success()};
}

}  // namespace forge
