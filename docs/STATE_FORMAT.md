# FST-1 State Format

FST-1 stores framed little-endian records with 8-byte alignment.

| Record | Meaning |
| --- | --- |
| `STATE_TXN_BEGIN` | Starts a private transaction with a transaction id, base sequence, and declared count. |
| `GRAPH_DELTA`, `INPUT_DELTA`, `JOB_DELTA` | Canonical mutation records. |
| `STATE_TXN_COMMIT` | Publishes the staged records and advances the state sequence. |
| `CHECKPOINT` | Records complete roots and reachability summaries. |

Replay applies only fully committed transactions. A partial tail or malformed record produces a diagnostic and leaves the previous committed prefix authoritative. Running or validating attempts without a committed success must be treated as lost after restart.
