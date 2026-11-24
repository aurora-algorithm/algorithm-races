# Recursive Fibonacci

Computes Fibonacci numbers by calling itself twice for every number, so the calls grow into a big tree.

- **Workload:** fib(40): 331 million calls
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20`, full = `40`
- **Expected checksum (`--quick`):** `6765` (full: `102334155`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 85 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 92 ms | 1.08x | 10 |
| 3 | Rust 1.99 | 172 ms | 2.02x | 10 |
| 4 | Goose 2026.4 · MSVC | 267 ms | 3.13x | 10 |
| 5 | C# on .NET 10 JIT | 297 ms | 3.48x | 10 |
| 6 | Java on HotSpot 25 | 308 ms | 3.62x | 10 |
| 7 | Go 1.27 | 336 ms | 3.94x | 10 |
| 8 | JavaScript on Node 24 | 579 ms | 6.80x | 10 |
| 9 | Python 3.14 | 8,570 ms | 100.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel fib

# 3. Full benchmark
python3 suite/run.py bench --kernel fib

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py fib --size 20 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
