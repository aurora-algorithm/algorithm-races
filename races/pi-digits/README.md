# Pi digits

Computes the first 500,000 digits of pi with the Chudnovsky formula, summed as a tree of exact fractions.

- **Workload:** 500,000 digits
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `10,000`, full = `500,000`
- **Expected checksum (`--quick`):** `10000:46101264836999892256959688159205600101655256375678:44897`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | JavaScript on Node 24 | 304 ms | 1.00x (fastest) | 10 |
| 2 | Go 1.27 | 385 ms | 1.26x | 10 |
| 3 | Rust 1.99 | 494 ms | 1.62x | 10 |
| 4 | Java on HotSpot 25 | 1,000 ms | 3.29x | 5 |
| 5 | Python 3.14 | 2,266 ms | 7.44x | 5 |
| 6 | C# on .NET 10 JIT | 6,810 ms | 22.4x | 3 |
| 7 | C++ with Boost, GCC 16 | 6,878 ms | 22.6x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/puzzle.mjs`](../../suite/js/puzzle.mjs) |
| Go | [`go.go`](go.go) | [`suite/go/puzzle.go`](../../suite/go/puzzle.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/puzzle.rs`](../../suite/rust/src/puzzle.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Puzzle.java`](../../suite/java/src/main/java/kernels/Puzzle.java) |
| Python | [`python.py`](python.py) | [`suite/python/puzzle.py`](../../suite/python/puzzle.py) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/PuzzleText.cs`](../../suite/csharp/PuzzleText.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/pi.cpp`](../../suite/cpp/src/pi.cpp) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel pi-digits

# 3. Full benchmark
python3 suite/run.py bench --kernel pi-digits

# 4. Or run one language directly (no harness)
cd suite/python && python3 main.py pi-digits --size 10000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
