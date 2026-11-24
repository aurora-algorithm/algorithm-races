# Spectral norm

Finds how far a 3,000 x 3,000 matrix can stretch a list of numbers, with 10 rounds of the power method.

- **Workload:** a 3,000 × 3,000 matrix
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100`, full = `3,000`
- **Expected checksum (`--quick`):** `1.274219991` (full: `1.274224153`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 284 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 284 ms | 1.00x | 10 |
| 3 | Go 1.27 | 285 ms | 1.00x | 10 |
| 4 | Rust 1.99 | 285 ms | 1.00x | 10 |
| 5 | Java on HotSpot 25 | 285 ms | 1.00x | 10 |
| 6 | Goose 2026.4 · MSVC | 288 ms | 1.01x | 10 |
| 7 | C# on .NET 10 JIT | 300 ms | 1.05x | 10 |
| 8 | JavaScript on Node 24 | 471 ms | 1.66x | 10 |
| 9 | Python 3.14 | 33,505 ms | 117.9x | 2 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/bg.c`](../../suite/c/src/bg.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/bg.cpp`](../../suite/cpp/src/bg.cpp) |
| Go | [`go.go`](go.go) | [`suite/go/bg.go`](../../suite/go/bg.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/bg.rs`](../../suite/rust/src/bg.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/BenchmarksGame.java`](../../suite/java/src/main/java/kernels/BenchmarksGame.java) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/bg.goose`](../../suite/goose/bg.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/BenchmarksGame.cs`](../../suite/csharp/BenchmarksGame.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/bg.mjs`](../../suite/js/bg.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/bg.py`](../../suite/python/bg.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel spectral-norm

# 3. Full benchmark
python3 suite/run.py bench --kernel spectral-norm

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py spectral-norm --size 100 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
