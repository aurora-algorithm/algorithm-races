# Gzip compression

Packs a 100 MiB mix of logs, JSON and source code at level 6.

- **Workload:** 100 MiB of logs, JSON and code
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup's file unpacked back to the same bytes
- **Harness sizes:** `--quick` = `1,048,576`, full = `104,857,600`
- **Expected checksum (`--quick`):** `1048576:764f5cd31bcc128ff2ae0a26051b094b5e099623fb4516eb0a87e4d735891cce`
- **Input data:** 100 MB mixed corpus generated automatically into `suite/data/` on first run.

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Go 1.27 | 555 ms | 1.00x (fastest) | 10 |
| 2 | C# on .NET 10 JIT | 848 ms | 1.53x | 10 |
| 3 | Python 3.14 | 889 ms | 1.60x | 10 |
| 4 | JavaScript on Node 24 | 1,144 ms | 2.06x | 9 |
| 5 | C with GCC 16 | 1,535 ms | 2.77x | 7 |
| 6 | C++ with GCC 16 | 1,554 ms | 2.80x | 7 |
| 7 | Java on HotSpot 25 | 1,692 ms | 3.05x | 6 |
| 8 | Rust 1.99 | 1,890 ms | 3.41x | 6 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |
| C | [`c.c`](c.c) | [`suite/c/src/work_compress.c`](../../suite/c/src/work_compress.c) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/compress.cpp`](../../suite/cpp/src/compress.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel gzip

# 3. Full benchmark
python3 suite/run.py bench --kernel gzip

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py gzip --size 1048576 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
