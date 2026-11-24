# Mandelbrot set

Repeats z = z squared + c for every pixel and colours the pixel by how fast it escapes.

- **Workload:** a 4,000 × 4,000 picture
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `200`, full = `4,000`
- **Expected checksum (`--quick`):** `3419197333` (full: `2418021795`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Java on HotSpot 25 | 660 ms | 1.00x (fastest) | 10 |
| 2 | Go 1.27 | 661 ms | 1.00x | 10 |
| 3 | C with GCC 16 | 663 ms | 1.00x | 10 |
| 4 | C++ with GCC 16 | 663 ms | 1.00x | 10 |
| 5 | Goose 2026.4 · MSVC | 669 ms | 1.01x | 10 |
| 6 | C# on .NET 10 JIT | 673 ms | 1.02x | 10 |
| 7 | Rust 1.99 | 676 ms | 1.02x | 10 |
| 8 | JavaScript on Node 24 | 773 ms | 1.17x | 10 |
| 9 | Python 3.14 | 27,964 ms | 42.4x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/BenchmarksGame.java`](../../suite/java/src/main/java/kernels/BenchmarksGame.java) |
| Go | [`go.go`](go.go) | [`suite/go/bg.go`](../../suite/go/bg.go) |
| C | [`c.c`](c.c) | [`suite/c/src/bg.c`](../../suite/c/src/bg.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/bg.cpp`](../../suite/cpp/src/bg.cpp) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/bg.goose`](../../suite/goose/bg.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/BenchmarksGame.cs`](../../suite/csharp/BenchmarksGame.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/bg.rs`](../../suite/rust/src/bg.rs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/bg.mjs`](../../suite/js/bg.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/bg.py`](../../suite/python/bg.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel mandelbrot

# 3. Full benchmark
python3 suite/run.py bench --kernel mandelbrot

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py mandelbrot --size 200 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
