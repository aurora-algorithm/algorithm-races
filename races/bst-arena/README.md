# Binary search tree in one array

Stores, finds and lists a million keys in a binary search tree whose nodes all sit in one array.

- **Workload:** 1,000,000 keys
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `20000:10000:240352578` (full: `1000000:500113:886730934`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 426 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 430 ms | 1.01x | 10 |
| 3 | Goose 2026.4 · MSVC | 448 ms | 1.05x | 10 |
| 4 | C with GCC 16 | 451 ms | 1.06x | 10 |
| 5 | Java on HotSpot 25 | 486 ms | 1.14x | 10 |
| 6 | C# on .NET 10 JIT | 512 ms | 1.20x | 10 |
| 7 | Go 1.27 | 534 ms | 1.25x | 10 |
| 8 | JavaScript on Node 24 | 647 ms | 1.52x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/arena.cpp`](../../suite/cpp/src/arena.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/arena.rs`](../../suite/rust/src/arena.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/arena.c`](../../suite/c/src/arena.c) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Arena.java`](../../suite/java/src/main/java/kernels/Arena.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Arena.cs`](../../suite/csharp/Arena.cs) |
| Go | [`go.go`](go.go) | [`suite/go/arena.go`](../../suite/go/arena.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/arena.mjs`](../../suite/js/arena.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bst-arena

# 3. Full benchmark
python3 suite/run.py bench --kernel bst-arena

# 4. Or run one language directly (no harness)
cd suite/rust && ./target/release/kernels bst-arena --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
