#include "forge/workspace.hpp"

namespace forge {

void VirtualFileSystem::write(std::string path, Bytes bytes) {
  files_[std::move(path)] = std::move(bytes);
}

void VirtualFileSystem::erase(std::string path) {
  files_.erase(path);
}

Result<Bytes> VirtualFileSystem::read(std::string_view path) const {
  auto it = files_.find(std::string(path));
  if (it == files_.end()) {
    return Status::error(ErrorCode::io, "virtual-fs", "file not found");
  }
  return it->second;
}

bool VirtualFileSystem::exists(std::string_view path) const {
  return files_.contains(std::string(path));
}

}  // namespace forge
