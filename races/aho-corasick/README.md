# Aho-Corasick

Finds every one of many patterns in a text while reading each letter only once.

- **Workload:** 10,000 patterns in 50,331,648 letters of DNA
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `1,048,576`, full = `50,331,648`
- **Expected checksum (`--quick`):** `10000:32440:493499:392019`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 197 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 197 ms | 1.00x | 10 |
| 3 | Go 1.27 | 211 ms | 1.07x | 10 |
| 4 | Java on HotSpot 25 | 314 ms | 1.59x | 10 |
| 5 | C# on .NET 10 JIT | 324 ms | 1.64x | 10 |
| 6 | Rust 1.99 | 342 ms | 1.73x | 10 |
| 7 | Goose 2026.4 · MSVC | 348 ms | 1.76x | 10 |
| 8 | JavaScript on Node 24 | 675 ms | 3.42x | 10 |
| 9 | Python 3.14 | 2,657 ms | 13.5x | 4 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle_text.cpp`](../../suite/cpp/src/puzzle_text.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle_text.c`](../../suite/c/src/puzzle_text.c) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle_text.go`](../../suite/go/puzzle_text.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/PuzzleText.java`](../../suite/java/src/main/java/kernels/PuzzleText.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/PuzzleText.cs`](../../suite/csharp/PuzzleText.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle_text.rs`](../../suite/rust/src/puzzle_text.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle_text.goose`](../../suite/goose/puzzle_text.goose) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle-text.mjs`](../../suite/js/puzzle-text.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel aho-corasick

# 3. Full benchmark
python3 suite/run.py bench --kernel aho-corasick

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py aho-corasick --size 1048576 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
