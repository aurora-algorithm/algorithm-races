# N-Queens backtracking

Places queens on a chessboard one row at a time and backs up when two queens attack each other.

- **Workload:** 13 queens on a 13 × 13 board
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `8`, full = `13`
- **Expected checksum (`--quick`):** `92` (full: `73712`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 179 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 185 ms | 1.04x | 10 |
| 3 | Go 1.27 | 200 ms | 1.12x | 10 |
| 4 | C++ with GCC 16 | 203 ms | 1.14x | 10 |
| 5 | C# on .NET 10 JIT | 206 ms | 1.15x | 10 |
| 6 | Goose 2026.4 · MSVC | 208 ms | 1.17x | 10 |
| 7 | Java on HotSpot 25 | 253 ms | 1.42x | 10 |
| 8 | JavaScript on Node 24 | 334 ms | 1.87x | 10 |
| 9 | Python 3.14 | 2,158 ms | 12.1x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel nqueens

# 3. Full benchmark
python3 suite/run.py bench --kernel nqueens

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py nqueens --size 8 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
