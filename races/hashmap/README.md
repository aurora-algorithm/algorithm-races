# Hash map

Stores 10 million keys, then looks every one up again.

- **Workload:** 10,000,000 keys
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `100000:50002` (full: `10000000:5011817`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 727 ms | 1.00x (fastest) | 10 |
| 2 | C# on .NET 10 JIT | 1,034 ms | 1.42x | 7 |
| 3 | Rust 1.99 | 1,064 ms | 1.46x | 8 |
| 4 | C with GCC 16 | 1,128 ms | 1.55x | 9 |
| 5 | Go 1.27 | 1,366 ms | 1.88x | 7 |
| 6 | Java on HotSpot 25 | 2,704 ms | 3.72x | 4 |
| 7 | Python 3.14 | 3,428 ms | 4.72x | 3 |
| 8 | C++ with GCC 16 | 3,516 ms | 4.84x | 3 |
| 9 | JavaScript on Node 24 | 5,811 ms | 7.99x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/misc.goose`](../../suite/goose/misc.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/lib.c`](../../suite/c/src/lib.c) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/misc.cpp`](../../suite/cpp/src/misc.cpp) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel hashmap

# 3. Full benchmark
python3 suite/run.py bench --kernel hashmap

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py hashmap --size 100000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
