# Forge

Forge is a C++20 local incremental build graph daemon core. It tracks versioned build nodes, declared and discovered dependencies, content fingerprints, duplicate-suppressed jobs, deterministic mock execution, content-addressed artifacts, FIPC-1 local protocol frames, and FST-1 state transactions.

This repository is an assisted implementation of `10_forge_incremental_build_graph_daemon.md`. It is not yet compiler-verified in this workspace because no CMake/C++ toolchain is available on `PATH`.

## Quick Build

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

If the host has Docker but no local C++ toolchain:

```sh
docker build -f Dockerfile.dev -t forge-dev .
docker run --rm -v "$PWD:/work" forge-dev cmake --preset debug
docker run --rm -v "$PWD:/work" forge-dev cmake --build --preset debug
docker run --rm -v "$PWD:/work" forge-dev ctest --preset debug --output-on-failure
```

## Safe Example

```sh
./build/debug/forge demo
./build/debug/forge graph apply examples/simple.fgm
./build/debug/forge build examples/simple.fgm
```

The demo creates an in-memory graph and runs the deterministic mock executor. It does not execute shell commands from graph input.

## Maturity

Core source, tests, fuzz harnesses, and docs are present. Toolchain-backed verification, Linux socket/inotify adapter validation, sanitizer campaigns, fuzz smoke runs, and performance measurements remain required before production use.
