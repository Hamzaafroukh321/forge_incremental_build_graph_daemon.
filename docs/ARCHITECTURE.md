# Architecture

Forge is split into a portable core and platform adapters.

- `FipcCodec` and `FipcSession` own frame parsing, CRC checks, stream generations, sequences, idempotency, and credit state.
- `WorkspaceCore` owns mutable workspace state and is the single event-loop owner in the MVP model.
- `BuildGraph` stores nodes and edges in generation-tagged maps and publishes immutable `GraphSnapshot` values by copy.
- `FileEventCoalescer` keeps one current token per normalized path and rejects stale fingerprint completions.
- `ExecutorManager` issues move-only worker authority through `WorkerLease` values and rejects stale attempt generations.
- `ArtifactStore` publishes immutable CAS objects only after digest verification.
- `StateCodec` and `AppendLog` encode FST-1 records and replay only committed transaction prefixes.
- `EventHub` applies byte credit and keeps terminal events even when progress is dropped.

Mutable workspace state is not thread-safe. Worker, watcher, and protocol completions must cross into the workspace with owned values and generation tokens.

## Lock Hierarchy

The portable core is written for single-owner workspace mutation. Future threaded adapters must follow:

1. Daemon lifecycle.
2. Workspace registry.
3. Artifact lease table.
4. Cache/index state.

Callbacks must not run while holding mutation locks.

## State Machines

The implemented state machines follow the specification: accepted to negotiated FIPC sessions, stream activation with generation reuse rules, file event pending/stabilizing completion tokens, job lease generation validation, terminal event retention, and committed-prefix recovery.
