# Kruskal's algorithm

Connects 2,000,000 points at the lowest total weight with union-find.

- **Workload:** 2,000,000 points
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `2,000,000`
- **Expected checksum (`--quick`):** `19993:3003704050` (full: `1999343:299746951188`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 296 ms | 1.00x (fastest) | 10 |
| 2 | Goose 2026.4 · MSVC | 320 ms | 1.08x | 10 |
| 3 | C++ with GCC 16 | 558 ms | 1.89x | 10 |
| 4 | C# on .NET 10 JIT | 723 ms | 2.44x | 6 |
| 5 | Go 1.27 | 960 ms | 3.24x | 10 |
| 6 | C with GCC 16 | 1,132 ms | 3.82x | 9 |
| 7 | Java on HotSpot 25 | 1,881 ms | 6.36x | 5 |
| 8 | JavaScript on Node 24 | 3,798 ms | 12.8x | 3 |
| 9 | Python 3.14 | 6,558 ms | 22.2x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_graph.cpp`](../../suite/cpp/src/algo_graph.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoGraphs.cs`](../../suite/csharp/AlgoGraphs.cs) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_graph.c`](../../suite/c/src/algo_graph.c) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoGraphs.java`](../../suite/java/src/main/java/kernels/AlgoGraphs.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel kruskal

# 3. Full benchmark
python3 suite/run.py bench --kernel kruskal

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py kruskal --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
