# Bloom filter

Answers whether a key was ever added using a few bits per key; it can say maybe, but it never misses a key that was added.

- **Workload:** 2,000,000 keys and 2,000,000 lookups
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `2,000,000`
- **Expected checksum (`--quick`):** `20000:100815:168`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 55 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 60 ms | 1.10x | 10 |
| 3 | Java on HotSpot 25 | 62 ms | 1.13x | 10 |
| 4 | Go 1.27 | 65 ms | 1.19x | 10 |
| 5 | C with GCC 16 | 67 ms | 1.24x | 10 |
| 6 | C# on .NET 10 JIT | 75 ms | 1.37x | 10 |
| 7 | Goose 2026.4 · MSVC | 79 ms | 1.44x | 10 |
| 8 | JavaScript on Node 24 | 235 ms | 4.31x | 10 |
| 9 | Python 3.14 | 5,104 ms | 93.5x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/puzzle.cpp`](../../suite/cpp/src/puzzle.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| C | [`c.c`](c.c) | [`suite/c/src/puzzle.c`](../../suite/c/src/puzzle.c) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Puzzle.cs`](../../suite/csharp/Puzzle.cs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/puzzle.goose`](../../suite/goose/puzzle.goose) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bloom

# 3. Full benchmark
python3 suite/run.py bench --kernel bloom

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py bloom --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
