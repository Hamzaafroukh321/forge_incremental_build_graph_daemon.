#include "forge/graph.hpp"

namespace forge {

GraphSnapshot::GraphSnapshot(Generation generation, std::map<NodeId, BuildNode> nodes, std::map<EdgeId, DependencyEdge> edges)
    : generation_(generation), nodes_(std::move(nodes)), edges_(std::move(edges)) {}

std::set<NodeId> GraphSnapshot::dependencies(NodeId node) const {
  std::set<NodeId> deps;
  for (const auto& [edge_id, edge] : edges_) {
    (void)edge_id;
    if (edge.from == node) {
      deps.insert(edge.to);
    }
  }
  return deps;
}

std::set<NodeId> GraphSnapshot::reverse_dependencies(NodeId node) const {
  std::set<NodeId> deps;
  for (const auto& [edge_id, edge] : edges_) {
    (void)edge_id;
    if (edge.to == node) {
      deps.insert(edge.from);
    }
  }
  return deps;
}

}  // namespace forge
