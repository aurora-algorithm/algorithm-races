# String join

Joins 10,000,000 short strings into one, with commas between.

- **Workload:** 10,000,000 parts
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `100,000`, full = `10,000,000`
- **Expected checksum (`--quick`):** `1074164:1691857237` (full: `107414072:3746706249`)

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose with append | 41 ms | 1.00x (fastest) | 10 |
| 2 | Go with strings.Join | 60 ms | 1.46x | 10 |
| 3 | Rust with slice.join | 62 ms | 1.51x | 10 |
| 4 | C# with string.Join | 78 ms | 1.90x | 10 |
| 5 | Python with str.join | 103 ms | 2.51x | 10 |
| 6 | Java with String.join | 156 ms | 3.82x | 10 |
| 7 | JavaScript with Node · Array.join | 175 ms | 4.29x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/ways.goose`](../../suite/goose/ways.goose) |
| Go | [`go.go`](go.go) | [`suite/go/ways.go`](../../suite/go/ways.go) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/ways.rs`](../../suite/rust/src/ways.rs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/WaysMaps.cs`](../../suite/csharp/WaysMaps.cs) |
| Python | [`python.py`](python.py) | [`suite/python/ways.py`](../../suite/python/ways.py) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Ways.java`](../../suite/java/src/main/java/kernels/Ways.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/ways.mjs`](../../suite/js/ways.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# 2. Quick verification for one implementation
python3 suite/run.py bench --quick --kernel bjoin.go.strings.Join

# 3. Or verify the entire builtins group across all built runtimes
python3 suite/run.py bench --quick --group builtins
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
