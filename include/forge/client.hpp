#pragma once

#include "forge/workspace.hpp"

namespace forge {

class Client {
 public:
  explicit Client(WorkspaceCore& workspace) : workspace_(workspace) {}
  [[nodiscard]] Result<NodeId> put_node(std::string name, ActionSpec action) { return workspace_.put_node(std::move(name), std::move(action)); }
  [[nodiscard]] Result<EdgeId> put_edge(NodeId from, NodeId to, EdgeKind kind = EdgeKind::declared) { return workspace_.put_edge(from, to, kind); }
  [[nodiscard]] Result<BuildOutcome> build(const std::vector<NodeId>& targets, Executor& executor) { return workspace_.build(targets, executor); }

 private:
  WorkspaceCore& workspace_;
};

}  // namespace forge
