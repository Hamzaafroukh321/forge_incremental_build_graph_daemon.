#pragma once

#include "forge/error.hpp"
#include "forge/ids.hpp"

#include <map>
#include <memory>
#include <set>

namespace forge {

enum class EdgeKind { declared, discovered };
enum class NodeState { clean, dirty, building, failed };

struct ActionSpec {
  std::string command;
  std::map<std::string, std::string> options;
};

struct BuildNode {
  NodeId id;
  Generation generation;
  std::string name;
  ActionSpec action;
  NodeState state{NodeState::dirty};
  Digest256 last_action_key{};
};

struct DependencyEdge {
  EdgeId id;
  Generation generation;
  NodeId from;
  NodeId to;
  EdgeKind kind{EdgeKind::declared};
};

struct CycleWitness {
  std::vector<NodeId> nodes;
  std::vector<EdgeId> edges;
};

class GraphSnapshot {
 public:
  GraphSnapshot() = default;
  GraphSnapshot(Generation generation, std::map<NodeId, BuildNode> nodes, std::map<EdgeId, DependencyEdge> edges);
  [[nodiscard]] Generation generation() const noexcept { return generation_; }
  [[nodiscard]] const std::map<NodeId, BuildNode>& nodes() const noexcept { return nodes_; }
  [[nodiscard]] const std::map<EdgeId, DependencyEdge>& edges() const noexcept { return edges_; }
  [[nodiscard]] std::set<NodeId> dependencies(NodeId node) const;
  [[nodiscard]] std::set<NodeId> reverse_dependencies(NodeId node) const;

 private:
  Generation generation_{};
  std::map<NodeId, BuildNode> nodes_;
  std::map<EdgeId, DependencyEdge> edges_;
};

class BuildGraph {
 public:
  [[nodiscard]] Result<NodeId> put_node(std::string name, ActionSpec action);
  [[nodiscard]] Result<EdgeId> put_edge(NodeId from, NodeId to, EdgeKind kind);
  [[nodiscard]] Result<void> remove_discovered_from(NodeId from);
  [[nodiscard]] Result<std::set<NodeId>> reverse_closure(const std::set<NodeId>& seeds) const;
  [[nodiscard]] std::optional<CycleWitness> cycle_witness() const;
  [[nodiscard]] GraphSnapshot snapshot() const;
  [[nodiscard]] Generation generation() const noexcept { return generation_; }
  [[nodiscard]] const std::map<NodeId, BuildNode>& nodes() const noexcept { return nodes_; }
  [[nodiscard]] const std::map<EdgeId, DependencyEdge>& edges() const noexcept { return edges_; }

 private:
  void publish_generation();
  std::uint64_t next_node_{1};
  std::uint64_t next_edge_{1};
  Generation generation_{};
  std::map<NodeId, BuildNode> nodes_;
  std::map<EdgeId, DependencyEdge> edges_;
};

Digest256 action_key_for(const BuildNode& node, const std::vector<Digest256>& input_keys, const std::vector<Digest256>& dependency_artifacts);

}  // namespace forge
