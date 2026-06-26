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

No numeric measurements are claimed in this environment because CMake and a C++ compiler are unavailable. Use `scripts/benchmark.py` after building on a supported host and record hardware, compiler, build type, and corpus.
