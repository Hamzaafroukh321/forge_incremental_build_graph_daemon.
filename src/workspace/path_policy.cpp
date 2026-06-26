#include "forge/workspace.hpp"

#include <sstream>

namespace forge {

PathPolicy::PathPolicy(std::filesystem::path root) : root_(std::filesystem::weakly_canonical(std::move(root))) {}

Result<NormalizedPath> PathPolicy::normalize(std::string_view path) const {
  if (path.empty()) {
    return Status::error(ErrorCode::path, "path-policy", "empty path");
  }
  std::filesystem::path input{std::string(path)};
  if (input.is_absolute()) {
    input = std::filesystem::relative(input, root_);
  }
  std::filesystem::path normalized;
  for (const auto& part : input.lexically_normal()) {
    const auto text = part.string();
    if (text == "." || text.empty()) {
      continue;
    }
    if (text == "..") {
      return Status::error(ErrorCode::path, "path-policy", "path escapes workspace");
    }
    normalized /= part;
  }
  auto out = normalized.generic_string();
  if (out.empty() || out.starts_with("../")) {
    return Status::error(ErrorCode::path, "path-policy", "path escapes workspace");
  }
  return NormalizedPath{out};
}

}  // namespace forge
