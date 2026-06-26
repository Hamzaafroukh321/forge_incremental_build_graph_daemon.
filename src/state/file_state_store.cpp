#include "forge/state.hpp"

#include <fstream>

namespace forge {
namespace {

constexpr std::uint64_t kCompactionTxnId = 0xC0FFEEU;

Result<Bytes> encode_transaction(std::uint64_t txn_id,
                                 std::uint64_t base_sequence,
                                 std::uint64_t new_sequence,
                                 std::vector<StateRecord> records) {
  AppendLog log;
  auto began = log.begin(txn_id, base_sequence, records.size());
  if (!began.ok()) {
    return began.status();
  }
  for (auto& record : records) {
    if (record.type == StateRecordType::txn_begin || record.type == StateRecordType::txn_commit) {
      return Status::error(ErrorCode::state_recovery, "file-state-store", "transaction control records are reserved");
    }
    record.txn_id = txn_id;
    auto appended = log.append(std::move(record));
    if (!appended.ok()) {
      return appended.status();
    }
  }
  auto committed = log.commit(txn_id, new_sequence);
  if (!committed.ok()) {
    return committed.status();
  }
  return log.bytes();
}

}  // namespace

FileStateStore::FileStateStore(std::filesystem::path root) : root_(std::move(root)) {}

Result<void> FileStateStore::append_transaction(std::uint64_t txn_id,
                                                std::uint64_t base_sequence,
                                                std::uint64_t new_sequence,
                                                std::vector<StateRecord> records) {
  auto existing = read_file(log_path());
  if (!existing.ok()) {
    return existing.status();
  }
  auto clean = clean_log_bytes(existing.value());
  if (!clean.ok()) {
    return clean.status();
  }
  if (clean.value().size() != existing.value().size()) {
    auto replaced = replace_file(log_path(), clean.value());
    if (!replaced.ok()) {
      return replaced.status();
    }
  }

  AppendLog log;
  log.replace_bytes(std::move(clean.value()));
  const auto old_size = log.bytes().size();
  auto began = log.begin(txn_id, base_sequence, records.size());
  if (!began.ok()) {
    return began.status();
  }
  for (auto& record : records) {
    if (record.type == StateRecordType::txn_begin || record.type == StateRecordType::txn_commit) {
      return Status::error(ErrorCode::state_recovery, "file-state-store", "transaction control records are reserved");
    }
    record.txn_id = txn_id;
    auto appended = log.append(std::move(record));
    if (!appended.ok()) {
      return appended.status();
    }
  }
  auto committed = log.commit(txn_id, new_sequence);
  if (!committed.ok()) {
    return committed.status();
  }

  const auto& bytes = log.bytes();
  Bytes tail(bytes.begin() + static_cast<std::ptrdiff_t>(old_size), bytes.end());
  return append_file(log_path(), tail);
}

Result<void> FileStateStore::checkpoint(std::uint64_t sequence, Bytes payload) {
  StateRecord record{StateRecordType::checkpoint, 0, 0, 0, std::move(payload)};
  auto encoded = encode_transaction(kCompactionTxnId, 0, sequence, {std::move(record)});
  if (!encoded.ok()) {
    return encoded.status();
  }
  auto replaced_checkpoint = replace_file(checkpoint_path(), encoded.value());
  if (!replaced_checkpoint.ok()) {
    return replaced_checkpoint.status();
  }
  return replace_file(log_path(), {});
}

Result<RecoveryResult> FileStateStore::recover() const {
  RecoveryResult result;
  auto checkpoint_bytes = read_file(checkpoint_path());
  if (!checkpoint_bytes.ok()) {
    return checkpoint_bytes.status();
  }
  if (!checkpoint_bytes.value().empty()) {
    auto checkpoint = recover_log(checkpoint_bytes.value());
    result.sequence = checkpoint.sequence;
    result.committed_records.insert(result.committed_records.end(),
                                    checkpoint.committed_records.begin(),
                                    checkpoint.committed_records.end());
    result.diagnostics.insert(result.diagnostics.end(), checkpoint.diagnostics.begin(), checkpoint.diagnostics.end());
  }

  auto log_bytes = read_file(log_path());
  if (!log_bytes.ok()) {
    return log_bytes.status();
  }
  if (!log_bytes.value().empty()) {
    auto log = recover_log(log_bytes.value());
    if (!log.committed_records.empty()) {
      result.sequence = log.sequence;
    }
    result.committed_records.insert(result.committed_records.end(), log.committed_records.begin(), log.committed_records.end());
    result.diagnostics.insert(result.diagnostics.end(), log.diagnostics.begin(), log.diagnostics.end());
  }
  return result;
}

std::filesystem::path FileStateStore::log_path() const {
  return root_ / "state.log";
}

std::filesystem::path FileStateStore::checkpoint_path() const {
  return root_ / "checkpoint.fst";
}

std::filesystem::path FileStateStore::temp_path(std::string_view name) const {
  return root_ / (std::string{name} + ".tmp");
}

Result<Bytes> FileStateStore::read_file(const std::filesystem::path& path) const {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return Bytes{};
  }
  if (ec) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to stat state file");
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to open state file");
  }
  Bytes bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if (input.bad()) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to read state file");
  }
  return bytes;
}

Result<void> FileStateStore::replace_file(const std::filesystem::path& path, const Bytes& bytes) const {
  std::error_code ec;
  std::filesystem::create_directories(root_, ec);
  if (ec) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to create state directory");
  }
  const auto temp = temp_path(path.filename().string());
  {
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if (!output) {
      return Status::error(ErrorCode::io, "file-state-store", "unable to open temporary state file");
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.flush();
    if (!output) {
      return Status::error(ErrorCode::io, "file-state-store", "unable to write temporary state file");
    }
  }
  std::filesystem::rename(temp, path, ec);
  if (ec) {
    std::filesystem::remove(path);
    ec.clear();
    std::filesystem::rename(temp, path, ec);
  }
  if (ec) {
    std::filesystem::remove(temp);
    return Status::error(ErrorCode::io, "file-state-store", "unable to publish state file");
  }
  return {};
}

Result<void> FileStateStore::append_file(const std::filesystem::path& path, const Bytes& bytes) const {
  std::error_code ec;
  std::filesystem::create_directories(root_, ec);
  if (ec) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to create state directory");
  }
  std::ofstream output(path, std::ios::binary | std::ios::app);
  if (!output) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to open append log");
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  output.flush();
  if (!output) {
    return Status::error(ErrorCode::io, "file-state-store", "unable to append state transaction");
  }
  return {};
}

Result<Bytes> FileStateStore::clean_log_bytes(const Bytes& bytes) const {
  if (bytes.empty()) {
    return Bytes{};
  }
  auto recovered = recover_log(bytes);
  if (recovered.diagnostics.empty()) {
    return bytes;
  }
  if (recovered.committed_records.empty()) {
    return Bytes{};
  }
  return encode_transaction(kCompactionTxnId, 0, recovered.sequence, recovered.committed_records);
}

}  // namespace forge
