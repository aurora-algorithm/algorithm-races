# Binary trees on 8 threads

Builds, counts and frees millions of small trees on 8 threads, one allocation per node.

- **Workload:** the depth 21 run
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `10`, full = `21`, threads = `[1, 2, 4, 8]`
- **Expected checksum (`--quick`):** `717034075` (full: `2503292173`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 665 ms | 1.00x (fastest) | 10 |
| 2 | JavaScript on Node 24 | 1,630 ms | 2.45x | 7 |
| 3 | Go 1.27 | 2,650 ms | 3.98x | 4 |
| 4 | C++ with GCC 16 | 4,465 ms | 6.71x | 3 |
| 5 | C with GCC 16 | 4,837 ms | 7.27x | 3 |
| 6 | Rust 1.99 | 5,101 ms | 7.67x | 3 |
| 7 | C# on .NET 10 JIT | 10,105 ms | 15.2x | 3 |
| 8 | Python 3.14 | 45,810 ms | 68.9x | 2 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/ParallelKernels.java`](../../suite/java/src/main/java/kernels/ParallelKernels.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/mt.mjs`](../../suite/js/mt.mjs) |
| Go | [`go.go`](go.go) | [`suite/go/mt.go`](../../suite/go/mt.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/mt.cpp`](../../suite/cpp/src/mt.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/mt.c`](../../suite/c/src/mt.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/mt.rs`](../../suite/rust/src/mt.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/ParallelKernels.cs`](../../suite/csharp/ParallelKernels.cs) |
| Python | [`python.py`](python.py) | [`suite/python/mt.py`](../../suite/python/mt.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel binary-trees-mt --threads 8

# 3. Full benchmark
python3 suite/run.py bench --kernel binary-trees-mt --threads 8  # omit --threads 8 to sweep [1, 2, 4, 8]

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py binary-trees-mt --size 10 --repeats 3 --threads 8
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
