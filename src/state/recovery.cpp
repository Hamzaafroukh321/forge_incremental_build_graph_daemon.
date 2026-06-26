#include "forge/state.hpp"

namespace forge {

RecoveryResult recover_log(const Bytes& bytes) {
  StateCodec codec;
  RecoveryResult result;
  std::size_t offset = 0;
  bool in_txn = false;
  std::uint64_t txn_id = 0;
  std::vector<StateRecord> staged;
  while (offset < bytes.size()) {
    Bytes tail(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end());
    std::size_t consumed = 0;
    auto decoded = codec.decode(tail, &consumed);
    if (!decoded.ok()) {
      result.diagnostics.push_back(decoded.status().message);
      break;
    }
    const StateRecord& record = decoded.value();
    if (record.type == StateRecordType::txn_begin) {
      in_txn = true;
      txn_id = record.txn_id;
      staged.clear();
    } else if (record.type == StateRecordType::txn_commit && in_txn && record.txn_id == txn_id) {
      result.committed_records.insert(result.committed_records.end(), staged.begin(), staged.end());
      result.sequence = record.new_sequence;
      in_txn = false;
      staged.clear();
    } else if (in_txn) {
      staged.push_back(record);
    }
    offset += consumed;
  }
  if (in_txn) {
    result.diagnostics.push_back("ignored partial transaction tail");
  }
  return result;
}

}  // namespace forge
