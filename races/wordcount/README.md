# Word count

Counts every word of a 1 GiB text file in a hash map.

- **Workload:** a 1 GiB text file
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `8,388,608`, full = `1,073,741,824`
- **Expected checksum (`--quick`):** `1201440:16256:bc:1048`
- **Input data:** 1 GiB text file generated automatically into `suite/data/` on first run.

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Goose 2026.4 · MSVC | 3,019 ms | 1.00x (fastest) | 4 |
| 2 | C with GCC 16 | 5,046 ms | 1.67x | 3 |
| 3 | Rust 1.99 | 5,309 ms | 1.76x | 3 |
| 4 | C++ with GCC 16 | 6,015 ms | 1.99x | 3 |
| 5 | Go 1.27 | 7,664 ms | 2.54x | 3 |
| 6 | C# on .NET 10 JIT | 8,853 ms | 2.93x | 3 |
| 7 | Java on HotSpot 25 | 10,024 ms | 3.32x | 3 |
| 8 | Python 3.14 | 16,603 ms | 5.50x | 3 |
| 9 | JavaScript on Node 24 | 20,346 ms | 6.74x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Goose | [`goose.goose`](goose.goose) | [`suite/goose/work.goose`](../../suite/goose/work.goose) |
| C | [`c.c`](c.c) | [`suite/c/src/work_files.c`](../../suite/c/src/work_files.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/work.cpp`](../../suite/cpp/src/work.cpp) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel wordcount

# 3. Full benchmark
python3 suite/run.py bench --kernel wordcount

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py wordcount --size 8388608 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
