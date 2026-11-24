# Sorting numbers already in order

Shows which sorts notice that the work is already done.

- **Workload:** 10,000,000 numbers already in order
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `240352578` (full: `2272776978`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java with Arrays.sort | 4.0 ms | 1.00x (fastest) | 10 |
| 2 | Rust with sort | 4.4 ms | 1.12x | 10 |
| 3 | Goose with sort | 4.7 ms | 1.19x | 10 |
| 4 | Go with slices.Sort | 5.5 ms | 1.39x | 10 |
| 5 | C++ with std::sort | 70 ms | 17.6x | 10 |
| 6 | C with qsort | 334 ms | 84.3x | 10 |
| 7 | JavaScript on Node 24 · sort | 338 ms | 85.3x | 10 |
| 8 | Python with list.sort | 360 ms | 91.0x | 10 |
| 9 | C# with Array.Sort | 362 ms | 91.4x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Ways.java`](../../suite/java/src/main/java/kernels/Ways.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/ways.cpp`](../../suite/cpp/src/ways.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/ways.c`](../../suite/c/src/ways.c) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Ways.cs`](../../suite/csharp/Ways.cs) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bsort-int-sorted.goose.sort

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
