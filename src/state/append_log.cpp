#include "forge/state.hpp"

namespace forge {

Result<void> AppendLog::begin(std::uint64_t txn_id, std::uint64_t base_sequence, std::uint64_t count) {
  Bytes payload;
  append_u64_le(payload, count);
  return append(StateRecord{StateRecordType::txn_begin, txn_id, base_sequence, 0, payload});
}

Result<void> AppendLog::append(StateRecord record) {
  Bytes encoded = codec_.encode(record);
  bytes_.insert(bytes_.end(), encoded.begin(), encoded.end());
  return {};
}

Result<void> AppendLog::commit(std::uint64_t txn_id, std::uint64_t new_sequence) {
  Bytes payload;
  append_u64_le(payload, crc32c(bytes_));
  return append(StateRecord{StateRecordType::txn_commit, txn_id, 0, new_sequence, payload});
}

Result<std::vector<StateRecord>> AppendLog::replay() const {
  return recover_log(bytes_).committed_records;
}

}  // namespace forge
