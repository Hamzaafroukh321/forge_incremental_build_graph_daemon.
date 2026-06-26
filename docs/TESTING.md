# Testing

The primary test executable is `forge_tests`. It contains named tests for:

- Error/result and checked arithmetic.
- FIPC frame splitting, CRC, stream generation, credit, and idempotency.
- Graph forward/reverse edges, snapshots, reverse closure, and cycles.
- Workspace coalescing, fingerprints, request planning, duplicate suppression, and cancellation.
- Worker lease generations and stale completion rejection.
- Artifact CAS publication, leases, and GC.
- FST committed-prefix recovery.
- Event-hub backpressure and terminal retention.
- Manifest-backed CLI graph/build workflow.
- Linux Unix-domain FIPC status round trip.
- Linux inotify file-change event smoke.

Run:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Sanitizer and TSan commands are in `AGENTS.md`.
