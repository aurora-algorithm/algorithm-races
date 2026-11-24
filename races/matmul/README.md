# Matrix multiplication

Multiplies two matrices, each answer cell summing a row times a column, while NumPy hands the whole job to BLAS, a math library tuned for the CPU cache and vector units.

- **Workload:** two 1,024 × 1,024 matrices
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `64`, full = `1,024`
- **Expected checksum (`--quick`):** `5431109:3906640011` (full: `21715745184:3524148976`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Python with NumPy 2.5 | 30 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 2,340 ms | 78.1x | 5 |
| 3 | Java on HotSpot 25 | 2,355 ms | 78.6x | 5 |
| 4 | Go 1.27 | 2,357 ms | 78.6x | 5 |
| 5 | Goose 2026.4 · MSVC | 2,385 ms | 79.6x | 4 |
| 6 | C# on .NET 10 JIT | 2,389 ms | 79.7x | 5 |
| 7 | JavaScript on Node 24 | 2,446 ms | 81.6x | 5 |
| 8 | C with GCC 16 | 2,590 ms | 86.4x | 4 |
| 9 | C++ with GCC 16 | 2,608 ms | 87.0x | 4 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/misc.goose`](../../suite/goose/misc.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |
| C | [`c.c`](c.c) | [`suite/c/src/loop.c`](../../suite/c/src/loop.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/misc.cpp`](../../suite/cpp/src/misc.cpp) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel matmul

# 3. Full benchmark
python3 suite/run.py bench --kernel matmul

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py matmul --size 64 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
