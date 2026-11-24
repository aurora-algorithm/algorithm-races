# Built-in sort

Shows what a built-in sort does inside: it splits around a pivot, finishes short pieces with insertion sort, or merges runs it finds.

- **Workload:** 10,000,000 numbers
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `240352578` (full: `2272776978`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose with sort | 166 ms | 1.00x (fastest) | 10 |
| 2 | Rust with sort | 211 ms | 1.27x | 10 |
| 3 | C++ with std::ranges::sort | 532 ms | 3.20x | 10 |
| 4 | Java with Arrays.sort | 556 ms | 3.34x | 10 |
| 5 | C# with Array.Sort | 560 ms | 3.36x | 9 |
| 6 | Go with slices.Sort | 655 ms | 3.94x | 10 |
| 7 | C with qsort | 1,203 ms | 7.23x | 8 |
| 8 | Python with list.sort | 2,704 ms | 16.2x | 4 |
| 9 | JavaScript on Node 24 · sort | 3,426 ms | 20.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/main.goose`](../../suite/goose/main.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/sorts.rs`](../../suite/rust/src/sorts.rs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/sorts.cpp`](../../suite/cpp/src/sorts.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Sorts.java`](../../suite/java/src/main/java/kernels/Sorts.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Sorts.cs`](../../suite/csharp/Sorts.cs) |
| Go | [`go.go`](go.go) | [`suite/go/main.go`](../../suite/go/main.go) |
| C | [`c.c`](c.c) | [`suite/c/src/sorts.c`](../../suite/c/src/sorts.c) |
| Python | [`python.py`](python.py) | [`suite/python/sorts.py`](../../suite/python/sorts.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/main.mjs`](../../suite/js/main.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel sort-int

# 3. Full benchmark
python3 suite/run.py bench --kernel sort-int

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py sort-int --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
