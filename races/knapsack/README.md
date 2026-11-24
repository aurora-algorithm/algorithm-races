# 0/1 knapsack

Fills a table of the best value for every weight limit, one item at a time.

- **Workload:** 1,000 items and a bag of 200,000
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `2,000`, full = `200,000`
- **Expected checksum (`--quick`):** `35343:1359398167` (full: `363993:2929089501`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 39 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 45 ms | 1.14x | 10 |
| 3 | C++ with GCC 16 | 45 ms | 1.14x | 10 |
| 4 | Goose 2026.4 · MSVC | 81 ms | 2.07x | 10 |
| 5 | Go 1.27 | 82 ms | 2.08x | 10 |
| 6 | Rust 1.99 | 87 ms | 2.21x | 10 |
| 7 | C# on .NET 10 JIT | 93 ms | 2.38x | 10 |
| 8 | JavaScript on Node 24 | 251 ms | 6.41x | 10 |
| 9 | Python 3.14 | 9,205 ms | 234.7x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel knapsack

# 3. Full benchmark
python3 suite/run.py bench --kernel knapsack

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py knapsack --size 2000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
