# Trie

Counts how many words start with each prefix by walking a tree of letters.

- **Workload:** 500,000 words and 500,000 prefixes
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `500,000`
- **Expected checksum (`--quick`):** `82211:4010954` (full: `1607089:2500770742`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Go 1.27 | 247 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 346 ms | 1.40x | 10 |
| 3 | C with GCC 16 | 350 ms | 1.42x | 10 |
| 4 | C++ with GCC 16 | 352 ms | 1.43x | 10 |
| 5 | Java on HotSpot 25 | 360 ms | 1.46x | 10 |
| 6 | JavaScript on Node 24 | 652 ms | 2.64x | 10 |
| 7 | C# on .NET 10 JIT | 957 ms | 3.88x | 10 |
| 8 | Python 3.14 | 1,973 ms | 8.00x | 6 |

## Caveats & language notes

- **goose:** Goose has no heap; see the -arena rows

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_tree.c`](../../suite/c/src/algo_tree.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_tree.cpp`](../../suite/cpp/src/algo_tree.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoTrees.java`](../../suite/java/src/main/java/kernels/AlgoTrees.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoTrees.cs`](../../suite/csharp/AlgoTrees.cs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel trie

# 3. Full benchmark
python3 suite/run.py bench --kernel trie

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py trie --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
