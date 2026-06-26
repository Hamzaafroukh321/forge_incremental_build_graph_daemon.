#include "forge/graph.hpp"

#include <algorithm>

namespace forge {

Result<NodeId> BuildGraph::put_node(std::string name, ActionSpec action) {
  if (name.empty()) {
    return Status::error(ErrorCode::graph_conflict, "graph", "node name is empty");
  }
  for (const auto& [id, node] : nodes_) {
    if (node.name == name) {
      BuildNode updated = node;
      updated.action = std::move(action);
      updated.generation = generation_.next();
      updated.state = NodeState::dirty;
      nodes_[id] = std::move(updated);
      publish_generation();
      return id;
    }
  }
  NodeId id{next_node_++};
  nodes_[id] = BuildNode{id, generation_.next(), std::move(name), std::move(action), NodeState::dirty, {}};
  publish_generation();
  return id;
}

Result<EdgeId> BuildGraph::put_edge(NodeId from, NodeId to, EdgeKind kind) {
  if (!nodes_.contains(from) || !nodes_.contains(to)) {
    return Status::error(ErrorCode::graph_conflict, "graph", "edge references unknown node");
  }
  for (const auto& [id, edge] : edges_) {
    if (edge.from == from && edge.to == to && edge.kind == kind) {
      return id;
    }
  }
  EdgeId id{next_edge_++};
  edges_[id] = DependencyEdge{id, generation_.next(), from, to, kind};
  nodes_[from].state = NodeState::dirty;
  publish_generation();
  return id;
}

Result<void> BuildGraph::remove_discovered_from(NodeId from) {
  for (auto it = edges_.begin(); it != edges_.end();) {
    if (it->second.from == from && it->second.kind == EdgeKind::discovered) {
      it = edges_.erase(it);
    } else {
      ++it;
    }
  }
  publish_generation();
  return {};
}

Result<std::set<NodeId>> BuildGraph::reverse_closure(const std::set<NodeId>& seeds) const {
  std::set<NodeId> reached = seeds;
  std::vector<NodeId> work(seeds.begin(), seeds.end());
  while (!work.empty()) {
    const NodeId node = work.back();
    work.pop_back();
    for (const auto& [edge_id, edge] : edges_) {
      (void)edge_id;
      if (edge.to == node && !reached.contains(edge.from)) {
        reached.insert(edge.from);
        work.push_back(edge.from);
      }
    }
  }
  return reached;
}

GraphSnapshot BuildGraph::snapshot() const {
  return GraphSnapshot(generation_, nodes_, edges_);
}

void BuildGraph::publish_generation() {
  generation_ = generation_.next();
}

}  // namespace forge
