# Binary search

Finds a number in a sorted list by checking the middle and keeping the half that can hold it.

- **Workload:** 2,000,000 lookups in 2,000,000 sorted numbers
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `2,000,000`
- **Expected checksum (`--quick`):** `8084:200713283` (full: `799895:1999758675683`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 197 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 253 ms | 1.29x | 10 |
| 3 | C++ with GCC 16 | 253 ms | 1.29x | 10 |
| 4 | Rust 1.99 | 255 ms | 1.30x | 10 |
| 5 | Go 1.27 | 263 ms | 1.34x | 10 |
| 6 | C# on .NET 10 JIT | 265 ms | 1.35x | 10 |
| 7 | Java on HotSpot 25 | 271 ms | 1.38x | 10 |
| 8 | JavaScript on Node 24 | 321 ms | 1.63x | 10 |
| 9 | Python 3.14 | 3,072 ms | 15.6x | 4 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel binary-search

# 3. Full benchmark
python3 suite/run.py bench --kernel binary-search

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py binary-search --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
