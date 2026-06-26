#pragma once

#include "forge/workspace.hpp"

#include <istream>
#include <map>

namespace forge {

struct ManifestNode {
  std::string name;
  ActionSpec action;
};

struct ManifestEdge {
  std::string from;
  std::string to;
  EdgeKind kind{EdgeKind::declared};
};

struct GraphManifest {
  std::vector<ManifestNode> nodes;
  std::vector<ManifestEdge> edges;
  std::vector<std::string> targets;
};

struct AppliedManifest {
  std::map<std::string, NodeId> nodes_by_name;
  std::vector<NodeId> targets;
};

[[nodiscard]] Result<GraphManifest> parse_manifest(std::istream& input);
[[nodiscard]] Result<AppliedManifest> apply_manifest(WorkspaceCore& workspace, const GraphManifest& manifest);

}  // namespace forge
