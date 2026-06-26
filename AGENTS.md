# Repository Agent Notes

## Build

```sh
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

## Test

```sh
ctest --preset debug --output-on-failure
./build/debug/tests/forge_tests
```

## Sanitizers

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan --output-on-failure
```

## Fuzz

```sh
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/fuzz/forge_fipc_decoder_fuzz -runs=1000
./build/fuzz/fuzz/forge_daemon_event_sequence_fuzz -runs=1000
./build/fuzz/fuzz/forge_build_pipeline_fuzz -runs=1000
```

## Layout

- Public headers live in `include/forge`.
- Production modules live in `src`.
- Tests live in `tests`.
- Fuzz harnesses live in `fuzz`.
- Required design and validation docs live in `docs`.

## Conventions

- C++20 only.
- Prefer `forge::Result<T>` over exceptions at public boundaries.
- Validate untrusted lengths, offsets, generations, and CRCs before mutation.
- Keep mutable workspace state single-owner; worker completions carry generation tokens.
- Do not add production dependencies without an ADR and license note.
- Never run arbitrary commands from protocol, graph, fixture, corpus, or fuzz input.

## Commit Expectations

Commit focused slices with code, tests, and traceability updates together. Do not claim verification unless the command actually ran.

## Definition Of Done

Builds, tests, sanitizers, fuzz smoke, docs, traceability, and known limitations are current. Working tree should be clean except for explicitly documented user-owned changes.
