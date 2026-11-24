# HyperLogLog

Estimates how many different items are in a stream of 12,000,000, using 16 KB of memory.

- **Workload:** 12,000,000 items
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `12,000,000`
- **Expected checksum (`--quick`):** `100000:24579:1576079745`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C# on .NET 10 JIT | 22 ms | 1.00x (fastest) | 10 |
| 2 | Java on HotSpot 25 | 23 ms | 1.02x | 10 |
| 3 | Go 1.27 | 23 ms | 1.03x | 10 |
| 4 | C++ with GCC 16 | 24 ms | 1.10x | 10 |
| 5 | Rust 1.99 | 26 ms | 1.15x | 10 |
| 6 | Goose 2026.4 · MSVC | 26 ms | 1.16x | 10 |
| 7 | C with GCC 16 | 37 ms | 1.69x | 10 |
| 8 | JavaScript on Node 24 | 93 ms | 4.17x | 10 |
| 9 | Python 3.14 | 6,722 ms | 302.3x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle.cpp`](../../suite/cpp/src/puzzle.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle.goose`](../../suite/goose/puzzle.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle.c`](../../suite/c/src/puzzle.c) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel hyperloglog

# 3. Full benchmark
python3 suite/run.py bench --kernel hyperloglog

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py hyperloglog --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
