# Merge sort

Sorts 10,000,000 numbers in 8 pieces on 8 threads and merges them back.

- **Workload:** 10,000,000 numbers
- **Reference machine:** Intel Core i7-13700F, Windows 11, 8 threads, one a core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `10,000,000`, threads = `[1, 2, 4, 8]`
- **Expected checksum (`--quick`):** `240352578` (full: `2272776978`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 152 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 161 ms | 1.06x | 10 |
| 3 | C++ with GCC 16 | 163 ms | 1.08x | 10 |
| 4 | Go 1.27 | 182 ms | 1.20x | 10 |
| 5 | C# on .NET 10 JIT | 206 ms | 1.36x | 10 |
| 6 | Java on HotSpot 25 | 229 ms | 1.51x | 10 |
| 7 | JavaScript on Node 24 | 293 ms | 1.93x | 10 |
| 8 | Python 3.14 | 23,496 ms | 155.0x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/mt.c`](../../suite/c/src/mt.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/mt.rs`](../../suite/rust/src/mt.rs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/mt.cpp`](../../suite/cpp/src/mt.cpp) |
| Go | [`go.go`](go.go) | [`suite/go/mt.go`](../../suite/go/mt.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/ParallelKernels.cs`](../../suite/csharp/ParallelKernels.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/ParallelKernels.java`](../../suite/java/src/main/java/kernels/ParallelKernels.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/mt.mjs`](../../suite/js/mt.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/mt.py`](../../suite/python/mt.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel mergesort-mt --threads 8

# 3. Full benchmark
python3 suite/run.py bench --kernel mergesort-mt --threads 8  # omit --threads 8 to sweep [1, 2, 4, 8]

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py mergesort-mt --size 20000 --repeats 3 --threads 8
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
