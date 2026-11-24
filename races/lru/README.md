# LRU cache

Sends 10,000,000 requests to a cache that keeps the 100,000 most recent keys.

- **Workload:** a cache of 100,000
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `49624:50376:2188093113` (full: `4969477:5030523:2867337861`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 302 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 314 ms | 1.04x | 10 |
| 3 | Goose 2026.4 · MSVC | 346 ms | 1.14x | 10 |
| 4 | C with GCC 16 | 631 ms | 2.09x | 10 |
| 5 | Go 1.27 | 673 ms | 2.23x | 10 |
| 6 | C# on .NET 10 JIT | 682 ms | 2.26x | 10 |
| 7 | C++ with GCC 16 | 1,104 ms | 3.65x | 9 |
| 8 | JavaScript on Node 24 | 1,515 ms | 5.01x | 7 |
| 9 | Python 3.14 | 2,169 ms | 7.18x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoTrees.java`](../../suite/java/src/main/java/kernels/AlgoTrees.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_tree.c`](../../suite/c/src/algo_tree.c) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoTrees.cs`](../../suite/csharp/AlgoTrees.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_tree.cpp`](../../suite/cpp/src/algo_tree.cpp) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel lru

# 3. Full benchmark
python3 suite/run.py bench --kernel lru

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py lru --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
