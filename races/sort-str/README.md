# Sorting words

Compares words letter by letter, and words that end up close share their first letters, so each check reads more.

- **Workload:** 1,000,000 words
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `3252989560` (full: `423755965`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust with sort | 121 ms | 1.00x (fastest) | 10 |
| 2 | Goose with sort | 144 ms | 1.19x | 10 |
| 3 | Go with slices.Sort | 149 ms | 1.22x | 10 |
| 4 | Python with list.sort | 189 ms | 1.56x | 10 |
| 5 | C with qsort | 232 ms | 1.91x | 10 |
| 6 | C++ with std::ranges::sort | 239 ms | 1.97x | 10 |
| 7 | C# with Array.Sort | 250 ms | 2.06x | 10 |
| 8 | Java with Arrays.sort | 302 ms | 2.49x | 10 |
| 9 | JavaScript on Node 24 · sort | 403 ms | 3.32x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/sorts.rs`](../../suite/rust/src/sorts.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/main.goose`](../../suite/goose/main.goose) |
| Go | [`go.go`](go.go) | [`suite/go/main.go`](../../suite/go/main.go) |
| Python | [`python.py`](python.py) | [`suite/python/sorts.py`](../../suite/python/sorts.py) |
| C | [`c.c`](c.c) | [`suite/c/src/sorts.c`](../../suite/c/src/sorts.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/sorts.cpp`](../../suite/cpp/src/sorts.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Sorts.cs`](../../suite/csharp/Sorts.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Main.java`](../../suite/java/src/main/java/kernels/Main.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/main.mjs`](../../suite/js/main.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel sort-str

# 3. Full benchmark
python3 suite/run.py bench --kernel sort-str

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py sort-str --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
