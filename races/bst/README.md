# Binary search tree

Inserts keys into a tree, smaller to the left and larger to the right, then searches it.

- **Workload:** 1,000,000 keys
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `20000:10000:240352578` (full: `1000000:500113:886730934`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 496 ms | 1.00x (fastest) | 10 |
| 2 | Go 1.27 | 604 ms | 1.22x | 10 |
| 3 | C++ with GCC 16 | 729 ms | 1.47x | 10 |
| 4 | C# on .NET 10 JIT | 768 ms | 1.55x | 10 |
| 5 | Rust 1.99 | 785 ms | 1.58x | 10 |
| 6 | C with GCC 16 | 862 ms | 1.74x | 10 |
| 7 | JavaScript on Node 24 | 1,287 ms | 2.59x | 7 |
| 8 | Python 3.14 | 3,561 ms | 7.18x | 3 |

## Caveats & language notes

- **goose:** Goose has no heap; see the -arena rows

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoTrees.java`](../../suite/java/src/main/java/kernels/AlgoTrees.java) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_tree.cpp`](../../suite/cpp/src/algo_tree.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoTrees.cs`](../../suite/csharp/AlgoTrees.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_tree.c`](../../suite/c/src/algo_tree.c) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bst

# 3. Full benchmark
python3 suite/run.py bench --kernel bst

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py bst --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
