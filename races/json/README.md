# JSON parsing

Reads 45.5 MB of JSON orders, adds up two fields and writes it all back.

- **Workload:** 450,000 orders, 45.5 MB
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `2,000`, full = `450,000`
- **Expected checksum (`--quick`):** `2000:50969000:10004:192525:2099231339` (full: `450000:11474775000:2250000:45517696:3651752632`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 90 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 183 ms | 2.04x | 10 |
| 3 | JavaScript on Node 24 | 262 ms | 2.93x | 10 |
| 4 | Java on HotSpot 25 | 331 ms | 3.69x | 10 |
| 5 | Go 1.27 | 465 ms | 5.19x | 10 |
| 6 | Python 3.14 | 705 ms | 7.86x | 10 |
| 7 | C with GCC 16 | 730 ms | 8.14x | 10 |
| 8 | C# on .NET 10 JIT | 948 ms | 10.6x | 7 |
| 9 | C++ with GCC 16 | 1,026 ms | 11.4x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/json_kernel.goose`](../../suite/goose/json_kernel.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |
| C | [`c.c`](c.c) | [`suite/c/src/lib.c`](../../suite/c/src/lib.c) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/json.cpp`](../../suite/cpp/src/json.cpp) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel json

# 3. Full benchmark
python3 suite/run.py bench --kernel json

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py json --size 2000 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
