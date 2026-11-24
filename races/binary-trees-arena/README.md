# Binary trees with an arena

Builds and drops millions of small trees with every node in one array, so a tree costs one allocation.

- **Workload:** the depth 21 run
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `10`, full = `21`
- **Expected checksum (`--quick`):** `717034075` (full: `2503292173`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 1,210 ms | 1.00x (fastest) | 8 |
| 2 | C with GCC 16 | 1,571 ms | 1.30x | 7 |
| 3 | Java on HotSpot 25 | 2,335 ms | 1.93x | 5 |
| 4 | C# on .NET 10 JIT | 2,344 ms | 1.94x | 3 |
| 5 | C++ with GCC 16 | 2,475 ms | 2.05x | 4 |
| 6 | Rust 1.99 | 2,840 ms | 2.35x | 4 |
| 7 | Go 1.27 | 2,963 ms | 2.45x | 4 |
| 8 | JavaScript on Node 24 | 7,182 ms | 5.94x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/bg.goose`](../../suite/goose/bg.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/arena.c`](../../suite/c/src/arena.c) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Arena.java`](../../suite/java/src/main/java/kernels/Arena.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Arena.cs`](../../suite/csharp/Arena.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/arena.cpp`](../../suite/cpp/src/arena.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/arena.rs`](../../suite/rust/src/arena.rs) |
| Go | [`go.go`](go.go) | [`suite/go/arena.go`](../../suite/go/arena.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/arena.mjs`](../../suite/js/arena.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel binary-trees-arena

# 3. Full benchmark
python3 suite/run.py bench --kernel binary-trees-arena

# 4. Or run one language directly (no harness)
cd suite/rust && ./target/release/kernels binary-trees-arena --size 10 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
