# Dijkstra's algorithm

Finds the shortest route from one node to all 2,000,000 nodes of a graph with 8,000,000 roads.

- **Workload:** 2,000,000 nodes
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `2,000,000`
- **Expected checksum (`--quick`):** `20000:4752196:463` (full: `2000000:712145521:682`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 400 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 481 ms | 1.20x | 10 |
| 3 | C with GCC 16 | 513 ms | 1.28x | 10 |
| 4 | C++ with GCC 16 | 558 ms | 1.39x | 10 |
| 5 | C# on .NET 10 JIT | 603 ms | 1.51x | 6 |
| 6 | Go 1.27 | 915 ms | 2.29x | 10 |
| 7 | Java on HotSpot 25 | 1,009 ms | 2.52x | 10 |
| 8 | JavaScript on Node 24 | 1,752 ms | 4.38x | 6 |
| 9 | Python 3.14 | 7,706 ms | 19.2x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_graph.c`](../../suite/c/src/algo_graph.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_graph.cpp`](../../suite/cpp/src/algo_graph.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoGraphs.cs`](../../suite/csharp/AlgoGraphs.cs) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoGraphs.java`](../../suite/java/src/main/java/kernels/AlgoGraphs.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel dijkstra

# 3. Full benchmark
python3 suite/run.py bench --kernel dijkstra

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py dijkstra --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
