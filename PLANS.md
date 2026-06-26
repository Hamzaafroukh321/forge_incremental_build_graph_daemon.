# Forge Implementation Plan

## Selected Specification

Governing specification: `10_forge_incremental_build_graph_daemon.md`.

Reasoning: it is the only top-level numbered architecture specification, it matches the repository name, and it contains the required project identity, fuzzing architecture, MVP acceptance criteria, and full-version acceptance criteria.

## Architecture Summary

Forge is implemented as a C++20 embeddable core plus local CLI/daemon entry points. The core owns a single workspace event-loop model, generation-tagged graph state, FIPC-1 frame/session validation, FST-1 transaction replay, deterministic file-event coalescing, action-key scheduling with duplicate suppression, a deterministic mock executor, a CAS artifact store, event crediting, and restart recovery semantics.

## Phases

| Phase | Work | Depends | Validation |
| --- | --- | --- | --- |
| 0 | Build system, warnings, sanitizer/fuzz presets, project control docs | none | configure/build commands in `AGENTS.md` |
| 1 | Error model, checked arithmetic, IDs/generations, digest primitives, limits | 0 | `forge_tests --suite base` |
| 2 | FIPC/FST codecs, canonical encoders, stream/session state | 1 | `forge_tests --suite ipc,state` |
| 3 | Graph, snapshots, reverse edges, SCC/cycle witnesses, invalidation | 2 | `forge_tests --suite graph` |
| 4 | Coalescer, virtual filesystem, fingerprints, action keys | 3 | `forge_tests --suite workspace` |
| 5 | Scheduler, requests, leases, mock executor, CAS publication | 4 | `forge_tests --suite scheduler,artifact` |
| 6 | Persistence/recovery, checkpoint/compaction, event hub, CLI workflows | 5 | integration tests and state checks |
| 7 | Fuzz harnesses, docs, benchmarks, final traceability audit | 6 | fuzz smoke, docs audit, benchmark command |

## Requirement Groups

- `REQ-BUILD`: CMake targets, presets, install/export smoke.
- `REQ-BASE`: errors, limits, checked arithmetic, IDs, digests.
- `REQ-FIPC`: FIPC-1 frame codec, negotiation, sequence, credit, idempotency.
- `REQ-FST`: FST-1 records, transaction commit/replay, checkpoint references.
- `REQ-GRAPH`: nodes, edges, generation snapshots, reverse closure, SCC witnesses.
- `REQ-WORKSPACE`: path policy, coalescing, stable fingerprints, invalidation reasons.
- `REQ-SCHED`: request closure, action keys, duplicate jobs, cancellation.
- `REQ-EXEC`: worker leases, deterministic mock executor, stale result rejection.
- `REQ-CAS`: artifact digest verification, durable publication, leases, GC.
- `REQ-EVENT`: status streams, terminal retention, byte credit/backpressure.
- `REQ-RECOVERY`: replay committed prefix, mark running attempts lost, compaction.
- `REQ-FUZZ`: three production-linked bounded deterministic fuzz harnesses.
- `REQ-DOCS`: required user, protocol, recovery, testing, performance, and provenance docs.

## Validation Commands

Expected commands on a machine with CMake and a C++20 compiler:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
cmake --preset release
cmake --build --preset release
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/forge_fipc_decoder_fuzz -runs=1000
./build/fuzz/forge_daemon_event_sequence_fuzz -runs=1000
./build/fuzz/forge_build_pipeline_fuzz -runs=1000
```

The current Windows host does not expose CMake or a C++ compiler on `PATH`, but Docker Desktop can run the `Dockerfile.dev` Linux toolchain. Docker debug, release, ASan/UBSan, fuzz, TSan, CLI, and install smoke commands are recorded in `docs/IMPLEMENTATION_STATUS.md`.

## Risks And Mitigation

| Risk | Mitigation |
| --- | --- |
| Spec scale exceeds a single development pass | Implement coherent production slices with honest traceability and blocked/unverified status. |
| Platform mismatch: spec targets Linux sockets/inotify, current environment is Windows | Keep core portable and isolate platform adapters; document Linux validation as required. |
| Recovery false success | Commit-success state only after CAS digest verification and transaction commit. |
| Stale generation publication | Every external completion carries a generation/lease token checked before mutation. |
| Fuzz harness drift | Fuzzers call production codec/workspace entry points and only decode operation bytes in harness code. |

## Definition Of Done

- All specification requirements are traceable to files and tests.
- Every public workflow has production code and named tests.
- FIPC/FST/action-key/recovery semantics are documented and implemented.
- Debug, release, sanitizer, and fuzz profiles build.
- Unit, integration, fuzz smoke, recovery, and benchmark commands have recorded outcomes.
- Remaining gaps are explicitly marked `Blocked` or `Unverified`, never silently represented as complete.

## Checklist

| Area | MVP | Full |
| --- | --- | --- |
| FIPC negotiated client/daemon | In progress | In progress |
| Atomic graph transactions and SCCs | In progress | In progress |
| File coalescing and fingerprints | In progress | In progress |
| Scheduler duplicate suppression | In progress | In progress |
| Deterministic mock executor | In progress | In progress |
| CAS publication | In progress | In progress |
| Append log/checkpoint/restart | In progress | In progress |
| 30 named tests | In progress | In progress |
| Three fuzz targets | Build and smoke verified | Build and smoke verified |
| TSan/concurrency campaigns | TSan unit harness verified in Docker with seccomp unconfined | Broader threaded stress still needed |
| Performance budgets | Unverified | Unverified |
