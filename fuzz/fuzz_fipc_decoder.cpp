#include "forge/fipc.hpp"

#include <cstdint>
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  forge::FipcSession session;
  std::size_t offset = 0;
  while (offset < size) {
    const std::size_t chunk = 1 + (data[offset] % 17);
    const std::size_t remaining = size - offset;
    const std::size_t actual = chunk < remaining ? chunk : remaining;
    auto result = session.feed(reinterpret_cast<const forge::Byte*>(data + offset), actual);
    if (!result.ok()) {
      return 0;
    }
    offset += actual;
  }
  return 0;
}

#ifndef FORGE_USE_LIBFUZZER
int main() {
  forge::FipcCodec codec;
  auto bytes = codec.encode(forge::FipcFrame{forge::FrameType::hello, 0, 0, 0, 0, 0, {}});
  LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
  std::cout << "forge_fipc_decoder_fuzz smoke ok\n";
  return 0;
}
#endif
