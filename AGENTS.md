# Repository Agent Notes

## Build

```sh
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

When the host has Docker Desktop but no local C++ toolchain:

```sh
docker build -f Dockerfile.dev -t forge-dev .
docker run --rm -v "$PWD:/work" forge-dev cmake --preset debug
docker run --rm -v "$PWD:/work" forge-dev cmake --build --preset debug
docker run --rm -v "$PWD:/work" forge-dev ctest --preset debug --output-on-failure
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
./build/fuzz/forge_fipc_decoder_fuzz
./build/fuzz/forge_daemon_event_sequence_fuzz
./build/fuzz/forge_build_pipeline_fuzz
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
