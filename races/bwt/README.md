# Burrows-Wheeler transform

Sorts every rotation of a DNA string and keeps the last column, the trick behind bzip2.

- **Workload:** 1,000,000 letters of DNA
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `20001:4:15857:2128877658`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 192 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 212 ms | 1.10x | 10 |
| 3 | Goose 2026.4 · MSVC | 215 ms | 1.11x | 10 |
| 4 | C with GCC 16 | 266 ms | 1.38x | 10 |
| 5 | C# on .NET 10 JIT | 356 ms | 1.85x | 10 |
| 6 | Java on HotSpot 25 | 402 ms | 2.09x | 9 |
| 7 | Go 1.27 | 491 ms | 2.55x | 10 |
| 8 | JavaScript on Node 24 | 521 ms | 2.70x | 10 |
| 9 | Python 3.14 | 2,920 ms | 15.2x | 4 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle_text.rs`](../../suite/rust/src/puzzle_text.rs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle_text.cpp`](../../suite/cpp/src/puzzle_text.cpp) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle_text.goose`](../../suite/goose/puzzle_text.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle_text.c`](../../suite/c/src/puzzle_text.c) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/PuzzleText.cs`](../../suite/csharp/PuzzleText.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/PuzzleText.java`](../../suite/java/src/main/java/kernels/PuzzleText.java) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle_text.go`](../../suite/go/puzzle_text.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle-text.mjs`](../../suite/js/puzzle-text.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bwt

# 3. Full benchmark
python3 suite/run.py bench --kernel bwt

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py bwt --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
