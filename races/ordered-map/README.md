# Ordered map

Adds a million keys to a sorted map, looks up a million more and reads them all back in order.

- **Workload:** 1,000,000 keys
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `20000:10000:240352578` (full: `1000000:500113:886730934`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 252 ms | 1.00x (fastest) | 10 |
| 2 | Java on HotSpot 25 | 852 ms | 3.38x | 10 |
| 3 | C++ with GCC 16 | 877 ms | 3.48x | 10 |
| 4 | C# on .NET 10 JIT | 1,038 ms | 4.12x | 7 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/algo.rs`](../../suite/rust/src/algo.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/AlgoTrees.java`](../../suite/java/src/main/java/kernels/AlgoTrees.java) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/algo_tree.cpp`](../../suite/cpp/src/algo_tree.cpp) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/AlgoTrees.cs`](../../suite/csharp/AlgoTrees.cs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel ordered-map

# 3. Full benchmark
python3 suite/run.py bench --kernel ordered-map

# 4. Or run one language directly (no harness)
cd suite/rust && ./target/release/kernels ordered-map --size 20000 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
