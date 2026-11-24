# Manacher's algorithm

Finds the longest palindrome around every position by reusing its mirror image.

- **Workload:** a string of 12,000,000 letters
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `12,000,000`
- **Expected checksum (`--quick`):** `100000:35:50913:300430`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C# on .NET 10 JIT | 152 ms | 1.00x (fastest) | 10 |
| 2 | Go 1.27 | 158 ms | 1.04x | 10 |
| 3 | C++ with GCC 16 | 164 ms | 1.08x | 10 |
| 4 | Java on HotSpot 25 | 166 ms | 1.09x | 10 |
| 5 | C with GCC 16 | 176 ms | 1.16x | 10 |
| 6 | Goose 2026.4 · MSVC | 191 ms | 1.26x | 10 |
| 7 | Rust 1.99 | 209 ms | 1.38x | 10 |
| 8 | JavaScript on Node 24 | 369 ms | 2.43x | 10 |
| 9 | Python 3.14 | 6,247 ms | 41.1x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/PuzzleText.cs`](../../suite/csharp/PuzzleText.cs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle_text.go`](../../suite/go/puzzle_text.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle_text.cpp`](../../suite/cpp/src/puzzle_text.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/PuzzleText.java`](../../suite/java/src/main/java/kernels/PuzzleText.java) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle_text.c`](../../suite/c/src/puzzle_text.c) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle_text.goose`](../../suite/goose/puzzle_text.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle_text.rs`](../../suite/rust/src/puzzle_text.rs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle-text.mjs`](../../suite/js/puzzle-text.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel palindrome

# 3. Full benchmark
python3 suite/run.py bench --kernel palindrome

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py palindrome --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
