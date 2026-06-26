#pragma once

#include "forge/artifact.hpp"
#include "forge/graph.hpp"

namespace forge {

enum class AttemptState { waiting, ready, running, validating, succeeded, failed, cancelled, lost };

struct WorkerLease {
  LeaseId id;
  JobId job;
  Generation attempt;
  bool active{true};
};

struct JobSpec {
  JobId job;
  NodeId node;
  Generation node_generation;
  Digest256 action_key;
  std::vector<std::string> declared_inputs;
};

struct WorkerResult {
  LeaseId lease;
  Generation attempt;
  bool success{false};
  std::vector<Bytes> artifacts;
  std::vector<std::pair<NodeId, NodeId>> discovered_edges;
  std::string diagnostic;
};

class Executor {
 public:
  virtual ~Executor() = default;
  [[nodiscard]] virtual Result<WorkerResult> run(const WorkerLease& lease, const JobSpec& spec) = 0;
  virtual void cancel(const WorkerLease& lease) = 0;
};

class DeterministicMockExecutor final : public Executor {
 public:
  [[nodiscard]] Result<WorkerResult> run(const WorkerLease& lease, const JobSpec& spec) override;
  void cancel(const WorkerLease& lease) override;
  void set_failure(NodeId node, std::string message);

 private:
  std::map<std::uint64_t, std::string> failures_;
  std::set<std::uint64_t> cancelled_;
};

class ExecutorManager {
 public:
  explicit ExecutorManager(Executor& executor);
  [[nodiscard]] WorkerLease lease(JobId job);
  [[nodiscard]] Result<WorkerResult> run(const WorkerLease& lease, const JobSpec& spec);
  [[nodiscard]] Result<void> validate_current(const WorkerLease& lease, const WorkerResult& result) const;
  void revoke(const WorkerLease& lease);

 private:
  Executor& executor_;
  std::uint64_t next_lease_{1};
  std::map<std::uint64_t, Generation> current_attempts_;
};

}  // namespace forge
