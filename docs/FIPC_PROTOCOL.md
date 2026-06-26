# FIPC-1 Protocol

FIPC-1 frames are little-endian packed byte records:

| Field | Size | Meaning |
| --- | ---: | --- |
| `frame_len` | 4 | Total header plus payload bytes, minimum 36. |
| `type` | 2 | Message code such as `HELLO`, `OPEN_STREAM`, `BUILD_REQUEST`, or `STATUS_EVENT`. |
| `flags` | 2 | Negotiated frame flags. |
| `stream_id` | 4 | Stream namespace; control stream is `0`. |
| `stream_generation` | 4 | Must increase when a stream id is reused. |
| `sequence` | 8 | Monotonic per stream. |
| `request_token` | 8 | Idempotency token. |
| `header_crc32c` | 4 | CRC32C over the fixed header before CRC fields. |
| `payload_crc32c` | 4 | CRC32C over payload. |
| `payload` | variable | Message bytes. |

Malformed length, CRC, stream generation, or sequence closes the offending session without dispatching a partial state mutation. Unknown required control messages are protocol errors; optional extension behavior must be negotiated before dispatch.

Credit is tracked per stream. Progress events may be dropped with an explicit gap, but terminal events are retained.
