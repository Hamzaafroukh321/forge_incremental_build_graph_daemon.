# Implementation Status

Generated from the local environment on 2026-06-26.

## Current Phase

Phase 0 through Phase 7 are represented in the repository with C++20 source, CMake targets, tests, fuzz harnesses, and documentation. Host compiler tools are not available on `PATH`, but Docker Desktop can run the `Dockerfile.dev` Linux toolchain image and now provides compiler-backed verification.

## Last Completed Ticket

- `FORGE-001` repository/build/test/fuzz structure drafted.
- `FORGE-002` through `FORGE-037` have initial production-linked implementations and tests, but are not marked verified until compiled and executed.
- `FORGE-009`, `FORGE-010`, `FORGE-012`, and `FORGE-013` were hardened in `ca4c2e8` with corrected FIPC frame layout, stream-generation retirement, FST declared-count/digest replay checks, and stricter test setup assertions.
- `FORGE-017` advanced with manifest-backed `forge graph apply` and `forge build` workflows, plus Docker-backed build/test/sanitizer/fuzz smoke evidence.
- `FORGE-026` advanced with filesystem-backed CAS publication and reopen verification.
- `FORGE-013` and `FORGE-028` advanced with filesystem-backed FST append, checkpoint rotation, restart replay, and crash-tail compaction.
- `FORGE-011` advanced with a Linux Unix-domain FIPC status socket, `forged --socket`, and `forge status --socket` smoke coverage.
- `FORGE-019` advanced with a Linux inotify watcher adapter and kernel event smoke coverage.

## Next Actionable Ticket

Implement long-running daemon lifecycle services, then run soak validation and performance budget checks.

## Completed Modules

- Project control docs.
- CMake target definitions and presets.
- Public headers for errors, IDs, codecs, graph, executor, artifacts, state, workspace, client, and daemon.
- Source modules for FIPC/FST, graph, scheduler, workspace coalescing, mock execution, CAS, event crediting, and CLI entry points.
- Named unit/integration tests and fuzz smoke harness sources. The unit harness currently registers 42 named tests.
- Documentation deliverables required by Section 24.

## In Progress Modules

- Long-running multi-client socket daemon lifecycle validation.
- Recursive watch registration and watcher lifecycle validation beyond the single-root inotify smoke.
- Long-running soak and performance budget evidence.

## Known Blockers

| Blocker | Affected requirements | Attempted resolution | Resume command |
| --- | --- | --- | --- |
| No CMake or C++ compiler available on local PATH | Native host builds | Built `Dockerfile.dev` image with GCC/Clang/CMake/Ninja and verified through Docker | Use Docker commands in `AGENTS.md`, or install host CMake/GCC/Clang/MSVC |
| Current host is Windows while spec targets Linux Unix-domain sockets and inotify-compatible adapters | Linux daemon runtime adapter validation | Kept core portable and verified Docker Linux Unix-domain socket and inotify smoke tests | Implement and run long-running socket/watcher validation on Linux x86-64/AArch64 |

## Exact Build And Test Status

- Host `cmake --version`: failed, command not found.
- Host `g++ --version`: failed, command not found.
- Host `clang++ --version`: failed, command not found.
- Host `cl`: failed, command not found.
- Docker `docker --context desktop-linux build -f Dockerfile.dev -t forge-dev .`: passed.
- Docker `cmake --preset debug`: passed.
- Docker `cmake --build --preset debug`: passed with warnings as errors.
- Docker `ctest --preset debug --output-on-failure`: passed, 1/1 CTest tests.
- Docker `cmake --preset release`: passed.
- Docker `cmake --build --preset release`: passed, including latest filesystem-backed CAS/state, socket adapter, and inotify watcher changes.
- Docker `forged --socket /tmp/forge-status.sock --once` plus `forge status --socket /tmp/forge-status.sock`: passed in debug and ASan builds.
- Docker `cmake --install build/release --prefix build/install-smoke`: passed.

Compiler-backed debug, release, tests, and install smoke have passed in the Docker Linux toolchain environment.

## Sanitizer Status

- Docker `cmake --preset asan`: passed.
- Docker `cmake --build --preset asan`: passed.
- Docker `ctest --preset asan --output-on-failure`: passed under ASan/UBSan.
- Docker GCC TSan build passed, but runtime failed with `ThreadSanitizer: unexpected memory mapping`.
- Docker Clang TSan build passed after adding `libclang-rt-18-dev`; direct run requires `--security-opt seccomp=unconfined` and passed under that mode.

## Fuzz Target Status

Sources exist for:

- `forge_fipc_decoder_fuzz`
- `forge_daemon_event_sequence_fuzz`
- `forge_build_pipeline_fuzz`

Docker fuzz preset builds and all three smoke executables passed under ASan/UBSan.

## Documentation Status

Required documentation files are present and now include Docker build/test instructions and manifest CLI examples. They still need a broader final audit against full-version behavior.

## Performance Status

Benchmark scripts and budgets are documented. Docker `scripts/benchmark.py` smoke passed with `forge demo elapsed_seconds=0.019242`, but full numeric criteria remain unverified.

## Deviations

| Decision | Record | Reason |
| --- | --- | --- |
| Core implementation is portable C++ and Linux-specific runtime adapters are isolated behind adapter classes | `docs/DECISIONS.md` ADR-0002 | Docker validates Unix-domain socket and inotify smoke coverage; long-running daemon behavior still needs target-platform campaigns. |
| Digest primitive uses an internal deterministic 256-bit keyed FNV-style combiner, not a cryptographic dependency | `docs/DECISIONS.md` ADR-0003 | Keep production dependency set zero until license/toolchain review. |

## Last Verified Commit

Latest compiler-verified committed baseline is `c10b054` (`feat(workspace): add Linux inotify watcher adapter`). Earlier implementation commits include `5de0292`, `3e58530`, `5022691`, `0774bb3`, `ca4c2e8`, `54aa070`, and `af0feb2`.
