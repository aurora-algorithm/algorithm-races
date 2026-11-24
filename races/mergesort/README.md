# Merge sort

Splits the array in halves and merges sorted halves back together.

- **Workload:** 10,000,000 numbers
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `240352578` (full: `2272776978`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 792 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 795 ms | 1.00x | 10 |
| 3 | Rust 1.99 | 818 ms | 1.03x | 10 |
| 4 | Goose 2026.4 · MSVC | 838 ms | 1.06x | 10 |
| 5 | Go 1.27 | 851 ms | 1.07x | 10 |
| 6 | C# on .NET 10 JIT | 915 ms | 1.15x | 8 |
| 7 | Java on HotSpot 25 | 944 ms | 1.19x | 10 |
| 8 | JavaScript on Node 24 | 1,434 ms | 1.81x | 7 |
| 9 | Python 3.14 | 21,712 ms | 27.4x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/sorts.c`](../../suite/c/src/sorts.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/sorts.cpp`](../../suite/cpp/src/sorts.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/sorts.rs`](../../suite/rust/src/sorts.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/sorts.goose`](../../suite/goose/sorts.goose) |
| Go | [`go.go`](go.go) | [`suite/go/sorts.go`](../../suite/go/sorts.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Sorts.cs`](../../suite/csharp/Sorts.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Sorts.java`](../../suite/java/src/main/java/kernels/Sorts.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/sorts.mjs`](../../suite/js/sorts.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/sorts.py`](../../suite/python/sorts.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel mergesort

# 3. Full benchmark
python3 suite/run.py bench --kernel mergesort

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py mergesort --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
