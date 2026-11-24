# Floyd's cycle detection

Sends a slow and a fast pointer along a sequence until they meet inside its loop.

- **Workload:** 4,000 starts mod 67,108,859
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20`, full = `4,000`
- **Expected checksum (`--quick`):** `20:183052:41900:16343`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 314 ms | 1.00x (fastest) | 10 |
| 2 | C# on .NET 10 JIT | 359 ms | 1.15x | 10 |
| 3 | Java on HotSpot 25 | 364 ms | 1.16x | 10 |
| 4 | C++ with GCC 16 | 365 ms | 1.16x | 10 |
| 5 | C with GCC 16 | 366 ms | 1.16x | 10 |
| 6 | Go 1.27 | 390 ms | 1.24x | 10 |
| 7 | Goose 2026.4 · MSVC | 402 ms | 1.28x | 10 |
| 8 | JavaScript on Node 24 | 1,153 ms | 3.67x | 9 |
| 9 | Python 3.14 | 7,712 ms | 24.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle.cpp`](../../suite/cpp/src/puzzle.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle.c`](../../suite/c/src/puzzle.c) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle.goose`](../../suite/goose/puzzle.goose) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel cycle

# 3. Full benchmark
python3 suite/run.py bench --kernel cycle

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py cycle --size 20 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
