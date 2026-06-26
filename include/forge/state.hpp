#pragma once

#include "forge/error.hpp"
#include "forge/graph.hpp"

namespace forge {

enum class StateRecordType : std::uint16_t {
  txn_begin = 0x1000,
  graph_delta = 0x1010,
  input_delta = 0x1011,
  job_delta = 0x1012,
  txn_commit = 0x10F0,
  checkpoint = 0x10F1
};

struct StateRecord {
  StateRecordType type{StateRecordType::txn_begin};
  std::uint64_t txn_id{0};
  std::uint64_t base_sequence{0};
  std::uint64_t new_sequence{0};
  Bytes payload;
};

class StateCodec {
 public:
  [[nodiscard]] Bytes encode(const StateRecord& record) const;
  [[nodiscard]] Result<StateRecord> decode(const Bytes& bytes, std::size_t* consumed) const;
};

class AppendLog {
 public:
  [[nodiscard]] Result<void> begin(std::uint64_t txn_id, std::uint64_t base_sequence, std::uint64_t count);
  [[nodiscard]] Result<void> append(StateRecord record);
  [[nodiscard]] Result<void> commit(std::uint64_t txn_id, std::uint64_t new_sequence);
  [[nodiscard]] Result<std::vector<StateRecord>> replay() const;
  [[nodiscard]] const Bytes& bytes() const noexcept { return bytes_; }
  void replace_bytes(Bytes bytes) { bytes_ = std::move(bytes); }

 private:
  StateCodec codec_;
  Bytes bytes_;
};

struct RecoveryResult {
  std::uint64_t sequence{0};
  std::vector<StateRecord> committed_records;
  std::vector<std::string> diagnostics;
};

[[nodiscard]] RecoveryResult recover_log(const Bytes& bytes);

}  // namespace forge
