# Sliding window maximum

Keeps a deque of candidates so the biggest value of every window comes out in one pass.

- **Workload:** 8,000,000 values, 5 window sizes
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `8,000,000`
- **Expected checksum (`--quick`):** `100000:3713810581:2068052765:2920804965:844921082:4294949870`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 263 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 268 ms | 1.02x | 10 |
| 3 | Goose 2026.4 · MSVC | 274 ms | 1.04x | 10 |
| 4 | C# on .NET 10 JIT | 281 ms | 1.07x | 10 |
| 5 | Go 1.27 | 282 ms | 1.07x | 10 |
| 6 | Rust 1.99 | 300 ms | 1.14x | 10 |
| 7 | Java on HotSpot 25 | 321 ms | 1.22x | 10 |
| 8 | JavaScript on Node 24 | 475 ms | 1.81x | 10 |
| 9 | Python 3.14 | 8,711 ms | 33.2x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle.cpp`](../../suite/cpp/src/puzzle.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle.c`](../../suite/c/src/puzzle.c) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle.goose`](../../suite/goose/puzzle.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel window-max

# 3. Full benchmark
python3 suite/run.py bench --kernel window-max

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py window-max --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
