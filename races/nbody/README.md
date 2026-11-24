# N-body simulation

Moves planets step by step by the gravity every one of them pulls on the others.

- **Workload:** 5,000,000 steps
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `1,000`, full = `5,000,000`
- **Expected checksum (`--quick`):** `-0.169075164;-0.169087605` (full: `-0.169075164;-0.169083134`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 147 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 160 ms | 1.09x | 10 |
| 3 | C++ with GCC 16 | 161 ms | 1.09x | 10 |
| 4 | C# on .NET 10 JIT | 177 ms | 1.20x | 10 |
| 5 | Java on HotSpot 25 | 181 ms | 1.23x | 10 |
| 6 | Goose 2026.4 · MSVC | 198 ms | 1.35x | 10 |
| 7 | JavaScript on Node 24 | 232 ms | 1.58x | 10 |
| 8 | Go 1.27 | 268 ms | 1.83x | 10 |
| 9 | Python 3.14 | 13,888 ms | 94.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/bg.rs`](../../suite/rust/src/bg.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/bg.c`](../../suite/c/src/bg.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/bg.cpp`](../../suite/cpp/src/bg.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/BenchmarksGame.cs`](../../suite/csharp/BenchmarksGame.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/BenchmarksGame.java`](../../suite/java/src/main/java/kernels/BenchmarksGame.java) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/bg.goose`](../../suite/goose/bg.goose) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/bg.mjs`](../../suite/js/bg.mjs) |
| Go | [`go.go`](go.go) | [`suite/go/bg.go`](../../suite/go/bg.go) |
| Python | [`python.py`](python.py) | [`suite/python/bg.py`](../../suite/python/bg.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel nbody

# 3. Full benchmark
python3 suite/run.py bench --kernel nbody

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py nbody --size 1000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
