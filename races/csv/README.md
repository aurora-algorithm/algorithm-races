# CSV parsing

Reads a 1 GB orders CSV and adds up what each customer spent.

- **Workload:** a 1 GB CSV
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `20,000`, full = `24,000,000`
- **Expected checksum (`--quick`):** `20000:20000:509940000:83983:50498`
- **Input data:** 1 GB orders CSV generated automatically into `suite/data/` on first run.

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C# on .NET 10 JIT | 664 ms | 1.00x (fastest) | 6 |
| 2 | Goose 2026.4 · MSVC | 1,480 ms | 2.23x | 6 |
| 3 | Rust 1.99 | 2,154 ms | 3.25x | 5 |
| 4 | C with GCC 16 | 2,865 ms | 4.32x | 4 |
| 5 | Go 1.27 | 4,819 ms | 7.26x | 3 |
| 6 | C++ with GCC 16 | 5,211 ms | 7.85x | 3 |
| 7 | Java on HotSpot 25 | 7,853 ms | 11.8x | 3 |
| 8 | Python 3.14 | 13,609 ms | 20.5x | 3 |
| 9 | JavaScript on Node 24 | 49,615 ms | 74.8x | 2 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/work.goose`](../../suite/goose/work.goose) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| C | [`c.c`](c.c) | [`suite/c/src/work_files.c`](../../suite/c/src/work_files.c) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/csv.cpp`](../../suite/cpp/src/csv.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel csv

# 3. Full benchmark
python3 suite/run.py bench --kernel csv

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py csv --size 20000 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
