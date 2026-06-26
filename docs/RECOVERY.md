# Recovery

Forge recovery is prefix-based. Append-log records are decoded in order; records between `STATE_TXN_BEGIN` and `STATE_TXN_COMMIT` are staged privately. Only a matching commit publishes staged records.

Failure points:

- Partial frame write: ignored during replay.
- Bad CRC: diagnostic, stop at last valid prefix.
- Durable artifact without state reference: harmless orphan.
- State success referencing missing CAS: safe inconsistency report; no success may be exposed.
- Running lease at restart: converted to lost unless committed success exists.

The current implementation includes CRC-checked in-memory FST replay plus a filesystem-backed state store. The store appends committed transactions to `state.log`, writes checkpoints through a temporary file and rename to `checkpoint.fst`, rotates the log after checkpoint publication, and compacts a corrupt or partial crash tail back to the last committed prefix before accepting new transactions.

Full fsync/barrier policy and Linux process restart validation still require target-platform verification.
