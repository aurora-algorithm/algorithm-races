# Binary trees

Builds, counts and frees millions of small trees, one allocation per node.

- **Workload:** the depth 21 run
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `10`, full = `21`
- **Expected checksum (`--quick`):** `717034075` (full: `2503292173`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 2,649 ms | 1.00x (fastest) | 4 |
| 2 | JavaScript on Node 24 | 6,803 ms | 2.57x | 3 |
| 3 | C# on .NET 10 JIT | 7,015 ms | 2.65x | 3 |
| 4 | Go 1.27 | 13,810 ms | 5.21x | 3 |
| 5 | C with GCC 16 | 15,348 ms | 5.79x | 3 |
| 6 | C++ with GCC 16 | 16,306 ms | 6.16x | 3 |
| 7 | Rust 1.99 | 17,094 ms | 6.45x | 3 |
| 8 | Python 3.14 | 42,293 ms | 16.0x | 2 |

## Caveats & language notes

- **goose:** Goose has no heap; see the -arena rows

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/BenchmarksGame.java`](../../suite/java/src/main/java/kernels/BenchmarksGame.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/bg.mjs`](../../suite/js/bg.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/BenchmarksGame.cs`](../../suite/csharp/BenchmarksGame.cs) |
| Go | [`go.go`](go.go) | [`suite/go/bg.go`](../../suite/go/bg.go) |
| C | [`c.c`](c.c) | [`suite/c/src/bg.c`](../../suite/c/src/bg.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/bg.cpp`](../../suite/cpp/src/bg.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/bg.rs`](../../suite/rust/src/bg.rs) |
| Python | [`python.py`](python.py) | [`suite/python/bg.py`](../../suite/python/bg.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel binary-trees

# 3. Full benchmark
python3 suite/run.py bench --kernel binary-trees

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py binary-trees --size 10 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
