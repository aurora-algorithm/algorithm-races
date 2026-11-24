# Sorting words already in order

Shows which sorts notice that the words are already in order.

- **Workload:** 1,000,000 words already in order
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `3252989560` (full: `423755965`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust with sort | 3.2 ms | 1.00x (fastest) | 10 |
| 2 | Go with slices.Sort | 6.5 ms | 2.05x | 10 |
| 3 | Java with Arrays.sort | 7.2 ms | 2.29x | 10 |
| 4 | Goose with sort | 7.3 ms | 2.32x | 10 |
| 5 | Python with list.sort | 30 ms | 9.33x | 10 |
| 6 | JavaScript on Node 24 · sort | 46 ms | 14.5x | 10 |
| 7 | C++ with std::sort | 78 ms | 24.8x | 10 |
| 8 | C# with Array.Sort | 104 ms | 32.9x | 10 |
| 9 | C with qsort | 107 ms | 33.9x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Ways.java`](../../suite/java/src/main/java/kernels/Ways.java) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/ways.cpp`](../../suite/cpp/src/ways.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Ways.cs`](../../suite/csharp/Ways.cs) |
| C | [`c.c`](c.c) | [`suite/c/src/ways.c`](../../suite/c/src/ways.c) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bsort-str-sorted.goose.sort

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
