#include "forge/graph.hpp"

#include <algorithm>
#include <functional>

namespace forge {

std::optional<CycleWitness> BuildGraph::cycle_witness() const {
  enum class Color { white, gray, black };
  std::map<NodeId, Color> color;
  std::vector<NodeId> stack;
  std::map<NodeId, EdgeId> parent_edge;
  for (const auto& [id, node] : nodes_) {
    (void)node;
    color[id] = Color::white;
  }
  std::function<std::optional<CycleWitness>(NodeId)> visit = [&](NodeId node) -> std::optional<CycleWitness> {
    color[node] = Color::gray;
    stack.push_back(node);
    for (const auto& [edge_id, edge] : edges_) {
      if (edge.from != node) {
        continue;
      }
      if (color[edge.to] == Color::white) {
        parent_edge[edge.to] = edge_id;
        if (auto found = visit(edge.to)) {
          return found;
        }
      } else if (color[edge.to] == Color::gray) {
        CycleWitness witness;
        auto start = std::find(stack.begin(), stack.end(), edge.to);
        witness.nodes.assign(start, stack.end());
        witness.nodes.push_back(edge.to);
        for (std::size_t i = 0; i + 1 < witness.nodes.size(); ++i) {
          for (const auto& [candidate_id, candidate] : edges_) {
            if (candidate.from == witness.nodes[i] && candidate.to == witness.nodes[i + 1]) {
              witness.edges.push_back(candidate_id);
              break;
            }
          }
        }
        return witness;
      }
    }
    stack.pop_back();
    color[node] = Color::black;
    return std::nullopt;
  };
  for (const auto& [id, node] : nodes_) {
    (void)node;
    if (color[id] == Color::white) {
      if (auto found = visit(id)) {
        return found;
      }
    }
  }
  return std::nullopt;
}

}  // namespace forge
