#include "forge/executor.hpp"

namespace forge {

Result<WorkerResult> DeterministicMockExecutor::run(const WorkerLease& lease, const JobSpec& spec) {
  if (cancelled_.contains(lease.id.value)) {
    return Status::error(ErrorCode::cancelled, "mock-executor", "lease cancelled");
  }
  if (auto failure = failures_.find(spec.node.value); failure != failures_.end()) {
    return WorkerResult{lease.id, lease.attempt, false, {}, {}, failure->second};
  }
  Bytes payload;
  const auto hex = spec.action_key.hex();
  for (char ch : hex) {
    payload.push_back(static_cast<Byte>(ch));
  }
  payload.push_back(0);
  payload.insert(payload.end(), spec.declared_inputs.size(), static_cast<Byte>(0x42));
  return WorkerResult{lease.id, lease.attempt, true, {payload}, {}, {}};
}

void DeterministicMockExecutor::cancel(const WorkerLease& lease) {
  cancelled_.insert(lease.id.value);
}

void DeterministicMockExecutor::set_failure(NodeId node, std::string message) {
  failures_[node.value] = std::move(message);
}

}  // namespace forge
