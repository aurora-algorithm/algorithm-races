# Breadth-first search

Floods a maze outward from the start one step at a time to find the shortest path.

- **Workload:** a 5,000 × 5,000 maze
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `200`, full = `5,000`
- **Expected checksum (`--quick`):** `27281:404:5787703` (full: `17204062:10012:89845005942`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 339 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 341 ms | 1.01x | 10 |
| 3 | Goose 2026.4 · MSVC | 357 ms | 1.05x | 10 |
| 4 | C# on .NET 10 JIT | 397 ms | 1.17x | 10 |
| 5 | Java on HotSpot 25 | 404 ms | 1.19x | 10 |
| 6 | Rust 1.99 | 468 ms | 1.38x | 10 |
| 7 | Go 1.27 | 642 ms | 1.90x | 10 |
| 8 | JavaScript on Node 24 | 836 ms | 2.47x | 10 |
| 9 | Python 3.14 | 8,538 ms | 25.2x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_graph.cpp`](../../suite/cpp/src/algo_graph.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_graph.c`](../../suite/c/src/algo_graph.c) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoGraphs.cs`](../../suite/csharp/AlgoGraphs.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoGraphs.java`](../../suite/java/src/main/java/kernels/AlgoGraphs.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bfs-grid

# 3. Full benchmark
python3 suite/run.py bench --kernel bfs-grid

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py bfs-grid --size 200 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
