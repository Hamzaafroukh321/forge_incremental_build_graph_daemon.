# Contributing

Use the specification tickets in `10_forge_incremental_build_graph_daemon.md` and update `docs/REQUIREMENTS_TRACEABILITY.md` with each feature.

Expectations:

- Keep changes focused.
- Add or update named tests with production behavior.
- Do not add dependencies without an ADR and license review.
- Do not claim sanitizer/fuzz/performance verification unless the command actually ran.
- Preserve assisted-development provenance.

Review checklist:

- Generation checks on external completions.
- Checked arithmetic for lengths and offsets.
- Deterministic ordering for graph, action keys, events, and replay.
- No hidden shell execution in tests or fuzzers.
- Documentation matches implemented behavior.
