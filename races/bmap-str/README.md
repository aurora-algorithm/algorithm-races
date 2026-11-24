# Hash map with string keys

Counts 1,000,000 strings and then looks 1,000,000 up.

- **Workload:** 1,000,000 strings
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `1,000,000`
- **Expected checksum (`--quick`):** `20000:10000` (full: `1000000:500000`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose with dictionary | 87 ms | 1.00x (fastest) | 10 |
| 2 | C# with Dictionary | 105 ms | 1.21x | 10 |
| 3 | Java with HashMap | 105 ms | 1.21x | 10 |
| 4 | Go with map | 182 ms | 2.09x | 10 |
| 5 | Rust with HashMap | 197 ms | 2.27x | 10 |
| 6 | Python with dict | 268 ms | 3.08x | 10 |
| 7 | JavaScript on Node 24 · Map | 338 ms | 3.89x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/WaysMaps.cs`](../../suite/csharp/WaysMaps.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/WaysMaps.java`](../../suite/java/src/main/java/kernels/WaysMaps.java) |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bmap-str.go.map

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
