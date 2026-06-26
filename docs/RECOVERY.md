# Recovery

Forge recovery is prefix-based. Append-log records are decoded in order; records between `STATE_TXN_BEGIN` and `STATE_TXN_COMMIT` are staged privately. Only a matching commit publishes staged records.

Failure points:

- Partial frame write: ignored during replay.
- Bad CRC: diagnostic, stop at last valid prefix.
- Durable artifact without state reference: harmless orphan.
- State success referencing missing CAS: safe inconsistency report; no success may be exposed.
- Running lease at restart: converted to lost unless committed success exists.

Current implementation provides in-memory FST replay logic. Filesystem fsync, atomic rename, checkpoint rotation, and Linux process restart validation require target-platform verification.
