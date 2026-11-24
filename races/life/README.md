# Conway's Game of Life

Updates a grid of cells over and over, each cell living or dying by how many neighbours it has.

- **Workload:** 512 × 512 cells, 100 generations
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `2`, full = `100`
- **Expected checksum (`--quick`):** `2:64528:1901743835`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 70 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 70 ms | 1.01x | 10 |
| 3 | Goose 2026.4 · MSVC | 103 ms | 1.48x | 10 |
| 4 | C# on .NET 10 JIT | 105 ms | 1.50x | 10 |
| 5 | Go 1.27 | 107 ms | 1.54x | 10 |
| 6 | Rust 1.99 | 115 ms | 1.64x | 10 |
| 7 | Java on HotSpot 25 | 120 ms | 1.71x | 10 |
| 8 | JavaScript on Node 24 | 155 ms | 2.22x | 10 |
| 9 | Python 3.14 | 6,289 ms | 90.0x | 3 |

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
python3 suite/run.py bench --quick --kernel life

# 3. Full benchmark
python3 suite/run.py bench --kernel life

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py life --size 2 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
