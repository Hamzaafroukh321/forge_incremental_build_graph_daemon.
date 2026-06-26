#include "forge/executor.hpp"

namespace forge {

ExecutorManager::ExecutorManager(Executor& executor) : executor_(executor) {}

WorkerLease ExecutorManager::lease(JobId job) {
  Generation attempt = current_attempts_[job.value].next();
  current_attempts_[job.value] = attempt;
  return WorkerLease{LeaseId{next_lease_++}, job, attempt, true};
}

Result<WorkerResult> ExecutorManager::run(const WorkerLease& lease, const JobSpec& spec) {
  auto valid = validate_current(lease, WorkerResult{lease.id, lease.attempt, false, {}, {}, {}});
  if (!valid.ok()) {
    return valid.status();
  }
  return executor_.run(lease, spec);
}

Result<void> ExecutorManager::validate_current(const WorkerLease& lease, const WorkerResult& result) const {
  if (!lease.active || result.lease != lease.id || result.attempt != lease.attempt) {
    return Status::error(ErrorCode::executor, "executor-manager", "stale worker lease");
  }
  auto it = current_attempts_.find(lease.job.value);
  if (it == current_attempts_.end() || it->second != lease.attempt) {
    return Status::error(ErrorCode::executor, "executor-manager", "attempt generation is not current");
  }
  return {};
}

void ExecutorManager::revoke(const WorkerLease& lease) {
  executor_.cancel(lease);
  current_attempts_.erase(lease.job.value);
}

}  // namespace forge
