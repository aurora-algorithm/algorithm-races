# Regex log search

Finds every failed GET or POST in a 2,000,000-line server log with one regex.

- **Workload:** 2,000,000 log lines
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `2,000,000`
- **Expected checksum (`--quick`):** `2514:126554869` (full: `250049:12495900210`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | JavaScript on Node 24 | 48 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 102 ms | 2.11x | 10 |
| 3 | Goose 2026.4 · MSVC | 149 ms | 3.11x | 10 |
| 4 | C with GCC 16 | 159 ms | 3.31x | 10 |
| 5 | Python 3.14 | 313 ms | 6.50x | 10 |
| 6 | C# on .NET 10 JIT | 388 ms | 8.07x | 10 |
| 7 | Java on HotSpot 25 | 491 ms | 10.2x | 10 |
| 8 | Go 1.27 | 1,865 ms | 38.8x | 6 |
| 9 | C++ with GCC 16 | 2,308 ms | 48.0x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/misc.goose`](../../suite/goose/misc.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/lib.c`](../../suite/c/src/lib.c) |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/misc.cpp`](../../suite/cpp/src/misc.cpp) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel regex-log

# 3. Full benchmark
python3 suite/run.py bench --kernel regex-log

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py regex-log --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
