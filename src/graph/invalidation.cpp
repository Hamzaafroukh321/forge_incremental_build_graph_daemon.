#include "forge/graph.hpp"

namespace forge {

Digest256 action_key_for(const BuildNode& node, const std::vector<Digest256>& input_keys, const std::vector<Digest256>& dependency_artifacts) {
  Bytes material;
  auto append_string = [&](std::string_view value) {
    append_u32_le(material, static_cast<std::uint32_t>(value.size()));
    for (char ch : value) {
      material.push_back(static_cast<Byte>(ch));
    }
  };
  append_string("forge-action-key-v1");
  append_string(node.action.command);
  for (const auto& [key, value] : node.action.options) {
    append_string(key);
    append_string(value);
  }
  for (const auto& digest : input_keys) {
    for (const auto word : digest.words) {
      append_u64_le(material, word);
    }
  }
  for (const auto& digest : dependency_artifacts) {
    for (const auto word : digest.words) {
      append_u64_le(material, word);
    }
  }
  return digest_bytes("action", material);
}

}  // namespace forge
