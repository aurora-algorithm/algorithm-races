# Sudoku solver

Counts every solution of a sudoku with dancing links, Knuth's backtracking on linked lists.

- **Workload:** 120 sudokus of 26 clues
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `4`, full = `120`
- **Expected checksum (`--quick`):** `4:15840:182788:1918894692`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 150 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 156 ms | 1.04x | 10 |
| 3 | Goose 2026.4 · MSVC | 182 ms | 1.21x | 10 |
| 4 | Java on HotSpot 25 | 190 ms | 1.27x | 10 |
| 5 | Rust 1.99 | 199 ms | 1.33x | 10 |
| 6 | Go 1.27 | 204 ms | 1.36x | 10 |
| 7 | JavaScript on Node 24 | 266 ms | 1.77x | 10 |
| 8 | C# on .NET 10 JIT | 493 ms | 3.29x | 10 |
| 9 | Python 3.14 | 6,523 ms | 43.5x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/sudoku.c`](../../suite/c/src/sudoku.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/sudoku.cpp`](../../suite/cpp/src/sudoku.cpp) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/sudoku.goose`](../../suite/goose/sudoku.goose) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| Python | [`python.py`](python.py) | [`suite/python/sudoku.py`](../../suite/python/sudoku.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel sudoku

# 3. Full benchmark
python3 suite/run.py bench --kernel sudoku

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py sudoku --size 4 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
