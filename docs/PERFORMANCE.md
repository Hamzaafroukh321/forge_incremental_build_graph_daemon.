# Performance

Specification budgets:

| Metric | MVP target |
| --- | ---: |
| Graph mutation | 50k edges/s |
| Small change-to-ready latency | <50 ms p95 after debounce |
| Scheduler throughput | 20k transitions/s |
| Daemon memory | <=512 MiB per 100k nodes |
| FIPC frame/backlog | 16 MiB frame / 8 MiB stream backlog |
| Fuzz speed | >25k FIPC/s and >1k events/s |

Docker smoke measurement on 2026-06-26:

```text
forge demo elapsed_seconds=0.019242
```

This only verifies that the benchmark harness runs in the Docker toolchain image. It does not satisfy the full throughput, latency, memory, or fuzz-speed budgets above. Full budget validation must record hardware, compiler, build type, corpus, command lines, and repeated-run statistics.
