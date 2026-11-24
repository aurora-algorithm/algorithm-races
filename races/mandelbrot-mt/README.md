# Mandelbrot fractal

Draws a 4,000 x 4,000 Mandelbrot picture with its rows dealt out in turn to 8 threads.

- **Workload:** a 4,000 × 4,000 picture
- **Reference machine:** Intel Core i7-13700F, Windows 11, 8 threads, one a core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `200`, full = `4,000`, threads = `[1, 2, 4, 8]`
- **Expected checksum (`--quick`):** `3419197333` (full: `2418021795`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 83 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 83 ms | 1.00x | 10 |
| 3 | C# on .NET 10 JIT | 85 ms | 1.03x | 10 |
| 4 | Java on HotSpot 25 | 91 ms | 1.10x | 10 |
| 5 | Rust 1.99 | 94 ms | 1.13x | 10 |
| 6 | Go 1.27 | 95 ms | 1.14x | 10 |
| 7 | JavaScript on Node 24 | 110 ms | 1.33x | 10 |
| 8 | Python 3.14 | 27,810 ms | 335.3x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/mt.c`](../../suite/c/src/mt.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/mt.cpp`](../../suite/cpp/src/mt.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/ParallelKernels.cs`](../../suite/csharp/ParallelKernels.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/ParallelKernels.java`](../../suite/java/src/main/java/kernels/ParallelKernels.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/mt.rs`](../../suite/rust/src/mt.rs) |
| Go | [`go.go`](go.go) | [`suite/go/mt.go`](../../suite/go/mt.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/mt.mjs`](../../suite/js/mt.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/mt.py`](../../suite/python/mt.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel mandelbrot-mt --threads 8

# 3. Full benchmark
python3 suite/run.py bench --kernel mandelbrot-mt --threads 8  # omit --threads 8 to sweep [1, 2, 4, 8]

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py mandelbrot-mt --size 200 --repeats 3 --threads 8
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
