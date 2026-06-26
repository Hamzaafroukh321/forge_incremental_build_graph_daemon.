# Requirements Traceability

Status meanings:

- `Implemented`: code/docs exist but have not been compiled in this environment.
- `Verified`: executable evidence exists and is listed.
- `Blocked`: external condition prevents verification or completion.

| ID | Specification section | Requirement | Implementation files | Test/fuzz evidence | Status | Commit |
| --- | --- | --- | --- | --- | --- | --- |
| REQ-BUILD-001 | 17, 19 | CMake core, CLI, tests, fuzz targets and presets | `CMakeLists.txt`, `CMakePresets.json`, `cmake/*` | Toolchain unavailable | Blocked | pending |
| REQ-BASE-001 | 6, 9, 12 | Stable IDs, generations, typed errors, checked arithmetic, limits | `include/forge/error.hpp`, `include/forge/ids.hpp`, `src/base/*` | `BaseErrorsAndCheckedArithmetic` | Implemented | pending |
| REQ-FIPC-001 | 7, 8, 10, 11 | FIPC-1 frame codec with CRC, caps, streaming, malformed errors | `include/forge/fipc.hpp`, `src/ipc/*` | `EveryByteFrameSplit`, `MalformedFrameRejected`, `forge_fipc_decoder_fuzz` | Implemented | pending |
| REQ-FIPC-002 | 8, 11 | Session negotiation, stream generation, sequence, credit, idempotency | `src/ipc/session.cpp`, `include/forge/fipc.hpp` | `HandshakeCapabilityNegotiation`, `StreamReuseNeedsGeneration`, `CreditNeverNegative`, `IdempotencyConflictAtomic` | Implemented | pending |
| REQ-FST-001 | 7, 10 | FST-1 canonical record writer/reader and committed-prefix replay | `include/forge/state.hpp`, `src/state/*` | `CrashEveryStateRecordBoundary`, `CheckpointLogRotationReplay` | Implemented | pending |
| REQ-GRAPH-001 | 6, 8, 10 | Build node/edge model, reverse edges, graph transactions | `include/forge/graph.hpp`, `src/graph/*` | `ForwardReverseEdgesAgree`, `GraphSnapshotSurvivesPublish` | Implemented | pending |
| REQ-GRAPH-002 | 10, 15 | Deterministic SCC/cycle witnesses | `src/graph/cycle_analyzer.cpp` | `DynamicCycleWitnessStable`, `DiscoveryCycleRejected` | Implemented | pending |
| REQ-WORKSPACE-001 | 8, 10, 15 | Path normalization, coalescing, stale fingerprint rejection | `include/forge/workspace.hpp`, `src/workspace/*` | `UnchangedContentNoInvalidation`, `StaleFingerprintIgnored`, `RenameDeleteCreateBurstCoalesces` | Implemented | pending |
| REQ-SCHED-001 | 8, 10, 15 | Request closure, readiness, action-key derivation | `include/forge/workspace.hpp`, `src/schedule/*` | `DiamondRunsEachNodeOnce`, `PriorityTieStable` | Implemented | pending |
| REQ-SCHED-002 | 8, 10, 15 | Duplicate work suppression and shared cancellation | `src/schedule/scheduler.cpp` | `SharedActionOneJob`, `CancelOneSharedRequest` | Implemented | pending |
| REQ-EXEC-001 | 8, 11, 14 | Worker leases and deterministic mock executor | `include/forge/executor.hpp`, `src/execute/*` | `LeaseGenerationRejectsLate`, `ExecutorTimeoutRetryGeneration` | Implemented | pending |
| REQ-CAS-001 | 4, 8, 10, 15 | Artifact digest verification, publication, leases, GC | `include/forge/artifact.hpp`, `src/artifact/*` | `ArtifactDigestMismatchQuarantined`, `MultiArtifactCommitAtomic`, `CasGcKeepsLeases` | Implemented | pending |
| REQ-EVENT-001 | 4, 8, 13, 15 | Credit-based event hub with terminal retention and progress gaps | `include/forge/event.hpp`, `src/event/*` | `SlowClientTerminalRetained`, `CreditNeverNegative` | Implemented | pending |
| REQ-RECOVERY-001 | 4, 7, 15 | Restart from committed checkpoint/log and mark running attempts lost | `src/state/recovery.cpp`, `include/forge/state.hpp` | `MissingCasBlocksRecoveredSuccess`, `WatcherRootRescanAfterRestart` | Implemented | pending |
| REQ-FUZZ-001 | 14 | FIPC decoder fuzz harness | `fuzz/fuzz_fipc_decoder.cpp` | build/smoke blocked | Blocked | pending |
| REQ-FUZZ-002 | 14 | Daemon event sequence fuzz harness | `fuzz/fuzz_daemon_events.cpp` | build/smoke blocked | Blocked | pending |
| REQ-FUZZ-003 | 14 | Full build pipeline fuzz harness | `fuzz/fuzz_build_pipeline.cpp` | build/smoke blocked | Blocked | pending |
| REQ-DOCS-001 | 24 | README, architecture, protocol, testing, fuzzing, recovery, security, contributing, changelog, performance | `README.md`, `docs/*.md`, `SECURITY.md`, `CONTRIBUTING.md`, `CHANGELOG.md` | Documentation audit pending after compile | Implemented | pending |
| REQ-PERF-001 | 18 | Benchmarks and budgets | `docs/PERFORMANCE.md`, `scripts/benchmark.py` | numeric run blocked | Blocked | pending |
