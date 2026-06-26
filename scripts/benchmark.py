#!/usr/bin/env python3
import subprocess
import time

start = time.perf_counter()
subprocess.run(["./build/release/forge", "demo"], check=True)
elapsed = time.perf_counter() - start
print(f"forge demo elapsed_seconds={elapsed:.6f}")
