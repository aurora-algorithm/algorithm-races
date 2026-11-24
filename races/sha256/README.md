# SHA-256 hashing

Hashes 1 GiB of data in memory.

- **Workload:** 1 GiB of data in memory
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `4,194,304`, full = `1,073,741,824`
- **Expected checksum (`--quick`):** `518d7d9e5db6e14d4d24bfbe424da18c038831bdb5ba3831b9f471a843a4bc2e` (full: `d2e4e26fbb42aab32553c6e4d5bfd596bf73a258be8066054135e2ffbb0a2bb4`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | JavaScript on Node 24 | 437 ms | 1.00x (fastest) | 10 |
| 2 | Rust 1.99 | 437 ms | 1.00x | 10 |
| 3 | C# on .NET 10 JIT | 438 ms | 1.00x | 10 |
| 4 | Python 3.14 | 439 ms | 1.00x | 10 |
| 5 | C++ with GCC 16 | 440 ms | 1.01x | 10 |
| 6 | Go 1.27 | 440 ms | 1.01x | 10 |
| 7 | Java on HotSpot 25 | 476 ms | 1.09x | 10 |
| 8 | C with GCC 16 | 2,353 ms | 5.39x | 5 |

## Caveats & language notes

- **c:** portable C, no SHA-NI: Mbed TLS 3.6.7 sha256.c has no x86 SHA extensions path (Rust, Go and C++ Botan use SHA-NI); OpenSSL has no official Windows build and building it needs Perl + NASM per compiler

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/misc.mjs`](../../suite/js/misc.mjs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/misc.rs`](../../suite/rust/src/misc.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Misc.cs`](../../suite/csharp/Misc.cs) |
| Python | [`python.py`](python.py) | [`suite/python/misc.py`](../../suite/python/misc.py) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/sha.cpp`](../../suite/cpp/src/sha.cpp) |
| Go | [`go.go`](go.go) | [`suite/go/misc.go`](../../suite/go/misc.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Misc.java`](../../suite/java/src/main/java/kernels/Misc.java) |
| C | [`c.c`](c.c) | [`suite/c/src/lib.c`](../../suite/c/src/lib.c) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel sha256

# 3. Full benchmark
python3 suite/run.py bench --kernel sha256

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py sha256 --size 4194304 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
