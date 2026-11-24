# Trapping rain water

Floods a terrain from its lowest wall inward with a min-heap and adds up the water held.

- **Workload:** a 1,500 × 1,500 map
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100`, full = `1,500`
- **Expected checksum (`--quick`):** `100:1411832:5177`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 163 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 180 ms | 1.10x | 10 |
| 3 | Goose 2026.4 · MSVC | 182 ms | 1.12x | 10 |
| 4 | C++ with GCC 16 | 194 ms | 1.19x | 10 |
| 5 | C# on .NET 10 JIT | 262 ms | 1.61x | 9 |
| 6 | JavaScript on Node 24 | 303 ms | 1.86x | 10 |
| 7 | Go 1.27 | 400 ms | 2.46x | 10 |
| 8 | Java on HotSpot 25 | 423 ms | 2.60x | 10 |
| 9 | Python 3.14 | 2,714 ms | 16.7x | 4 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle.c`](../../suite/c/src/puzzle.c) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle.goose`](../../suite/goose/puzzle.goose) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle.cpp`](../../suite/cpp/src/puzzle.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel rain-water

# 3. Full benchmark
python3 suite/run.py bench --kernel rain-water

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py rain-water --size 100 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
