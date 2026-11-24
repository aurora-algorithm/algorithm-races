# Edit distance

Finds the fewest edits between two strings of 10,000 letters, filling 100 million cells.

- **Workload:** a 10,000 x 10,000 grid
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `500`, full = `10,000`
- **Expected checksum (`--quick`):** `259` (full: `5161`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 62 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 64 ms | 1.03x | 10 |
| 3 | Rust 1.99 | 100 ms | 1.62x | 10 |
| 4 | Goose 2026.4 · MSVC | 163 ms | 2.64x | 10 |
| 5 | Go 1.27 | 181 ms | 2.93x | 10 |
| 6 | Java on HotSpot 25 | 199 ms | 3.22x | 10 |
| 7 | C# on .NET 10 JIT | 237 ms | 3.84x | 10 |
| 8 | JavaScript on Node 24 | 504 ms | 8.15x | 10 |
| 9 | Python 3.14 | 10,172 ms | 164.7x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel edit-distance

# 3. Full benchmark
python3 suite/run.py bench --kernel edit-distance

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py edit-distance --size 500 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
