# Radix sort

Sorts numbers by their bytes instead of comparing them.

- **Workload:** 10,000,000 numbers below 2^32 held as 64-bit integers
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `240352578` (full: `2272776978`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Go with zermelo | 101 ms | 1.00x (fastest) | 10 |
| 2 | Rust with voracious_sort | 107 ms | 1.06x | 10 |
| 3 | C# with HPCsharp radix | 120 ms | 1.19x | 10 |
| 4 | Goose with sort | 168 ms | 1.66x | 10 |
| 5 | C++ with Boost spreadsort | 280 ms | 2.78x | 10 |
| 6 | Java with Arrays.sort | 554 ms | 5.49x | 10 |
| 7 | C with qsort | 1,203 ms | 11.9x | 9 |
| 8 | Python with list.sort | 2,694 ms | 26.7x | 4 |
| 9 | JavaScript with Array.sort | 3,410 ms | 33.8x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Ways.cs`](../../suite/csharp/Ways.cs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/ways.cpp`](../../suite/cpp/src/ways.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Ways.java`](../../suite/java/src/main/java/kernels/Ways.java) |
| C | [`c.c`](c.c) | [`suite/c/src/ways.c`](../../suite/c/src/ways.c) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bsort-int.go.zermelo.Sort

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
