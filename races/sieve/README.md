# Sieve of Eratosthenes

Finds every prime number up to a limit by crossing out the multiples of each prime.

- **Workload:** the numbers up to 100,000,000
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `1,000,000`, full = `100,000,000`
- **Expected checksum (`--quick`):** `78498` (full: `5761455`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C++ with GCC 16 | 489 ms | 1.00x (fastest) | 10 |
| 2 | C with GCC 16 | 490 ms | 1.00x | 10 |
| 3 | Goose 2026.4 · MSVC | 493 ms | 1.01x | 10 |
| 4 | Go 1.27 | 494 ms | 1.01x | 10 |
| 5 | Rust 1.99 | 509 ms | 1.04x | 10 |
| 6 | Java on HotSpot 25 | 518 ms | 1.06x | 10 |
| 7 | C# on .NET 10 JIT | 556 ms | 1.14x | 10 |
| 8 | JavaScript on Node 24 | 580 ms | 1.19x | 10 |
| 9 | Python 3.14 | 5,700 ms | 11.7x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/misc.cpp`](../../suite/cpp/src/misc.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/loop.c`](../../suite/c/src/loop.c) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/misc.goose`](../../suite/goose/misc.goose) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel sieve

# 3. Full benchmark
python3 suite/run.py bench --kernel sieve

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py sieve --size 1000000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
