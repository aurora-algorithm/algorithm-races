# Huffman coding

Builds a tree from letter counts so common letters get short codes.

- **Workload:** 16 MiB of text
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `1,048,576`, full = `16,777,216`
- **Expected checksum (`--quick`):** `1048576:4207370:354843703:1`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 256 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 269 ms | 1.05x | 10 |
| 3 | Rust 1.99 | 288 ms | 1.13x | 10 |
| 4 | Java on HotSpot 25 | 297 ms | 1.16x | 10 |
| 5 | Goose 2026.4 · MSVC | 304 ms | 1.19x | 10 |
| 6 | C# on .NET 10 JIT | 337 ms | 1.32x | 10 |
| 7 | Go 1.27 | 348 ms | 1.36x | 10 |
| 8 | JavaScript on Node 24 | 656 ms | 2.57x | 10 |
| 9 | Python 3.14 | 6,027 ms | 23.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle_text.cpp`](../../suite/cpp/src/puzzle_text.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle_text.c`](../../suite/c/src/puzzle_text.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle_text.rs`](../../suite/rust/src/puzzle_text.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/PuzzleText.java`](../../suite/java/src/main/java/kernels/PuzzleText.java) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle_text.goose`](../../suite/goose/puzzle_text.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/PuzzleText.cs`](../../suite/csharp/PuzzleText.cs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle_text.go`](../../suite/go/puzzle_text.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle-text.mjs`](../../suite/js/puzzle-text.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel huffman

# 3. Full benchmark
python3 suite/run.py bench --kernel huffman

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py huffman --size 1048576 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
