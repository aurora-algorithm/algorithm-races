# Fannkuch

Flips stacks of pancakes for every ordering and finds the stack that needs the most flips.

- **Workload:** an 11-pancake stack
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `7`, full = `11`
- **Expected checksum (`--quick`):** `228:16` (full: `556355:51`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 1,511 ms | 1.00x (fastest) | 7 |
| 2 | Go 1.27 | 1,672 ms | 1.11x | 7 |
| 3 | Goose 2026.4 · MSVC | 1,701 ms | 1.13x | 6 |
| 4 | Rust 1.99 | 1,748 ms | 1.16x | 6 |
| 5 | C++ with GCC 16 | 1,903 ms | 1.26x | 6 |
| 6 | C# on .NET 10 JIT | 2,071 ms | 1.37x | 5 |
| 7 | Java on HotSpot 25 | 2,092 ms | 1.38x | 5 |
| 8 | JavaScript on Node 24 | 2,096 ms | 1.39x | 5 |
| 9 | Python 3.14 | 43,726 ms | 28.9x | 2 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/bg.c`](../../suite/c/src/bg.c) |
| Go | [`go.go`](go.go) | [`suite/go/bg.go`](../../suite/go/bg.go) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/bg.goose`](../../suite/goose/bg.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/bg.rs`](../../suite/rust/src/bg.rs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/bg.cpp`](../../suite/cpp/src/bg.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/BenchmarksGame.cs`](../../suite/csharp/BenchmarksGame.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/BenchmarksGame.java`](../../suite/java/src/main/java/kernels/BenchmarksGame.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/bg.mjs`](../../suite/js/bg.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/bg.py`](../../suite/python/bg.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel fannkuch

# 3. Full benchmark
python3 suite/run.py bench --kernel fannkuch

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py fannkuch --size 7 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
