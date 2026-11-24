# KMP string search

Finds every place an 8-letter pattern appears in 50 million letters of DNA.

- **Workload:** 50,000,000 letters of DNA
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `50,000,000`
- **Expected checksum (`--quick`):** `3:28175:50000` (full: `778:13035:49953462`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 122 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 135 ms | 1.11x | 10 |
| 3 | Rust 1.99 | 144 ms | 1.18x | 10 |
| 4 | Goose 2026.4 · MSVC | 149 ms | 1.22x | 10 |
| 5 | C# on .NET 10 JIT | 149 ms | 1.22x | 10 |
| 6 | Go 1.27 | 169 ms | 1.39x | 10 |
| 7 | Java on HotSpot 25 | 192 ms | 1.58x | 10 |
| 8 | JavaScript on Node 24 | 294 ms | 2.42x | 10 |
| 9 | Python 3.14 | 2,485 ms | 20.4x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/algo_dp.c`](../../suite/c/src/algo_dp.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_misc.cpp`](../../suite/cpp/src/algo_misc.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/algo.goose`](../../suite/goose/algo.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Algo.cs`](../../suite/csharp/Algo.cs) |
| Go | [`go.go`](go.go) | [`suite/go/algo.go`](../../suite/go/algo.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Algo.java`](../../suite/java/src/main/java/kernels/Algo.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/algo.mjs`](../../suite/js/algo.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/algo.py`](../../suite/python/algo.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel kmp

# 3. Full benchmark
python3 suite/run.py bench --kernel kmp

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py kmp --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
