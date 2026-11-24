# Trie in one array

Counts how many words start with each prefix, using a trie whose nodes all sit in one array.

- **Workload:** 500,000 words
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `500,000`
- **Expected checksum (`--quick`):** `82211:4010954` (full: `1607089:2500770742`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 28 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 92 ms | 3.25x | 10 |
| 3 | C# on .NET 10 JIT | 94 ms | 3.32x | 10 |
| 4 | C++ with GCC 16 | 98 ms | 3.47x | 10 |
| 5 | Rust 1.99 | 101 ms | 3.57x | 10 |
| 6 | Java on HotSpot 25 | 140 ms | 4.92x | 10 |
| 7 | JavaScript on Node 24 | 200 ms | 7.03x | 10 |
| 8 | Go 1.27 | 210 ms | 7.38x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/arena.c`](../../suite/c/src/arena.c) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Arena.cs`](../../suite/csharp/Arena.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/arena.cpp`](../../suite/cpp/src/arena.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/arena.rs`](../../suite/rust/src/arena.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Arena.java`](../../suite/java/src/main/java/kernels/Arena.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/arena.mjs`](../../suite/js/arena.mjs) |
| Go | [`go.go`](go.go) | [`suite/go/arena.go`](../../suite/go/arena.go) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel trie-arena

# 3. Full benchmark
python3 suite/run.py bench --kernel trie-arena

# 4. Or run one language directly (no harness)
cd suite/rust && ./target/release/kernels trie-arena --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
