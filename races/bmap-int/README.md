# Hash map with number keys

Counts 10,000,000 numbers and then looks 10,000,000 up.

- **Workload:** 10,000,000 numbers
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `100000:50002` (full: `10000000:5011817`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose with dictionary | 763 ms | 1.00x (fastest) | 10 |
| 2 | C# with Dictionary | 1,009 ms | 1.32x | 8 |
| 3 | Rust with HashMap | 1,049 ms | 1.37x | 9 |
| 4 | Go with map | 1,324 ms | 1.74x | 8 |
| 5 | Java with HashMap | 2,673 ms | 3.50x | 4 |
| 6 | Python with dict | 3,380 ms | 4.43x | 3 |
| 7 | JavaScript on Node 24 · Map | 5,792 ms | 7.59x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/WaysMaps.cs`](../../suite/csharp/WaysMaps.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/WaysMaps.java`](../../suite/java/src/main/java/kernels/WaysMaps.java) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bmap-int.go.map

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
