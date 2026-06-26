#include "forge/workspace.hpp"

#include <algorithm>

namespace forge {

WorkspaceCore::WorkspaceCore(std::filesystem::path root) : path_policy_(std::move(root)) {}

Result<NodeId> WorkspaceCore::put_node(std::string name, ActionSpec action) {
  return graph_.put_node(std::move(name), std::move(action));
}

Result<EdgeId> WorkspaceCore::put_edge(NodeId from, NodeId to, EdgeKind kind) {
  return graph_.put_edge(from, to, kind);
}

Result<void> WorkspaceCore::file_event(std::string_view path, FileEventKind kind) {
  auto normalized = path_policy_.normalize(path);
  if (!normalized.ok()) {
    return normalized.status();
  }
  auto event = coalescer_.push(normalized.value(), kind, clock_.now());
  if (!event.ok()) {
    return event.status();
  }
  clock_.advance(5);
  auto fingerprint = fingerprint_path(event.value().path);
  if (!fingerprint.ok()) {
    return fingerprint.status();
  }
  auto completed = coalescer_.complete(event.value(), InputFingerprint{event.value().path, fingerprint.value(), 0, {}});
  if (!completed.ok()) {
    return completed.status();
  }
  return {};
}

Result<BuildOutcome> WorkspaceCore::build(const std::vector<NodeId>& targets, Executor& executor) {
  if (auto cycle = graph_.cycle_witness()) {
    (void)cycle;
    return Status::error(ErrorCode::cycle, "workspace", "graph contains prohibited cycle");
  }
  auto order = plan_order(targets);
  if (!order.ok()) {
    return order.status();
  }
  RequestId request{next_request_++};
  ExecutorManager manager(executor);
  BuildOutcome outcome{request, {}, {}};
  auto opened = event_hub_.open_stream(static_cast<std::uint32_t>(request.value), 1024 * 1024);
  if (!opened.ok()) {
    return opened.status();
  }
  std::map<std::string, JobId> active_by_key;
  for (NodeId node_id : order.value()) {
    const auto node_it = graph_.nodes().find(node_id);
    if (node_it == graph_.nodes().end()) {
      return Status::error(ErrorCode::graph_conflict, "workspace", "target missing");
    }
    const Digest256 key = action_key_for(node_it->second, {}, {});
    const std::string key_hex = key.hex();
    JobId job{0};
    auto active = active_by_key.find(key_hex);
    if (active != active_by_key.end()) {
      job = active->second;
      outcome.events.push_back({EventKind::queued, "attached duplicate job", false, 24});
      continue;
    }
    job = JobId{next_job_++};
    active_by_key[key_hex] = job;
    WorkerLease lease = manager.lease(job);
    JobSpec spec{job, node_id, node_it->second.generation, key, {}};
    outcome.events.push_back({EventKind::running, "running " + node_it->second.name, false, 16});
    auto result = manager.run(lease, spec);
    if (!result.ok()) {
      outcome.events.push_back({EventKind::failed, result.status().message, true, 16});
      return outcome;
    }
    auto valid = manager.validate_current(lease, result.value());
    if (!valid.ok()) {
      outcome.events.push_back({EventKind::failed, valid.status().message, true, 16});
      return outcome;
    }
    for (const auto& artifact_bytes : result.value().artifacts) {
      ArtifactWriter writer;
      writer.append(artifact_bytes.data(), artifact_bytes.size());
      auto published = artifacts_.publish(std::move(writer));
      if (!published.ok()) {
        outcome.events.push_back({EventKind::failed, published.status().message, true, 16});
        return outcome;
      }
      outcome.artifacts.push_back(published.value());
    }
    outcome.events.push_back({EventKind::succeeded, "succeeded " + node_it->second.name, true, 16});
  }
  return outcome;
}

Result<Digest256> WorkspaceCore::fingerprint_path(const NormalizedPath& path) {
  auto bytes = fs_.read(path.relative);
  if (!bytes.ok()) {
    Bytes tombstone;
    return digest_bytes("missing-input", tombstone);
  }
  return digest_bytes("input", bytes.value());
}

Result<std::vector<NodeId>> WorkspaceCore::plan_order(const std::vector<NodeId>& targets) const {
  GraphSnapshot snapshot = graph_.snapshot();
  std::set<NodeId> seen;
  std::vector<NodeId> order;
  std::function<void(NodeId)> visit = [&](NodeId node) {
    if (seen.contains(node)) {
      return;
    }
    seen.insert(node);
    for (NodeId dep : snapshot.dependencies(node)) {
      visit(dep);
    }
    order.push_back(node);
  };
  for (NodeId target : targets) {
    if (!snapshot.nodes().contains(target)) {
      return Status::error(ErrorCode::graph_conflict, "workspace", "unknown target");
    }
    visit(target);
  }
  return order;
}

}  // namespace forge
