# Fuzzing

Forge includes three production-linked fuzz harnesses:

- `forge_fipc_decoder_fuzz`: feeds arbitrary socket chunks into `FipcSession::feed`.
- `forge_daemon_event_sequence_fuzz`: maps bytes to graph, file event, time, and build operations on `WorkspaceCore`.
- `forge_build_pipeline_fuzz`: combines FIPC handshake frames with workspace graph/build operations.

Harnesses use deterministic mock execution and virtual adapters. They do not execute shell commands from input.

Smoke commands:

```sh
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/forge_fipc_decoder_fuzz -runs=1000
./build/fuzz/forge_daemon_event_sequence_fuzz -runs=1000
./build/fuzz/forge_build_pipeline_fuzz -runs=1000
```

When a crash is found, save the exact input under `tests/regression/`, add a named unit or integration test that exercises the production path, and keep the minimized corpus input only as evidence.
