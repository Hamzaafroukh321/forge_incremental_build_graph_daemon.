#include "forge/state.hpp"

namespace forge {

RecoveryResult recover_log(const Bytes& bytes) {
  StateCodec codec;
  RecoveryResult result;
  std::size_t offset = 0;
  bool in_txn = false;
  std::uint64_t txn_id = 0;
  std::uint64_t declared_count = 0;
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
      auto count = read_u64_le(record.payload, 0);
      if (!count.ok()) {
        result.diagnostics.push_back("transaction begin missing declared count");
        break;
      }
      in_txn = true;
      txn_id = record.txn_id;
      declared_count = count.value();
      staged.clear();
    } else if (record.type == StateRecordType::txn_commit && in_txn && record.txn_id == txn_id) {
      auto expected_crc = read_u64_le(record.payload, 0);
      if (!expected_crc.ok()) {
        result.diagnostics.push_back("transaction commit missing digest");
        break;
      }
      if (staged.size() != declared_count) {
        result.diagnostics.push_back("ignored transaction with mismatched declared record count");
        in_txn = false;
        staged.clear();
        offset += consumed;
        continue;
      }
      if (crc32c(bytes.data(), offset) != static_cast<std::uint32_t>(expected_crc.value())) {
        result.diagnostics.push_back("ignored transaction with commit digest mismatch");
        in_txn = false;
        staged.clear();
        offset += consumed;
        continue;
      }
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
