# Zstd decompression

Unpacks a zstd file back to 100 MiB of logs, JSON and source code.

- **Workload:** 100 MiB of logs, JSON and code
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup got back the same 100 MiB
- **Harness sizes:** `--quick` = `1,048,576`, full = `104,857,600`
- **Expected checksum (`--quick`):** `1048576:764f5cd31bcc128ff2ae0a26051b094b5e099623fb4516eb0a87e4d735891cce`
- **Input data:** 100 MB mixed corpus generated automatically into `suite/data/` on first run.

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Go 1.27 | 60 ms | 1.00x (fastest) | 10 |
| 2 | C++ with GCC 16 | 76 ms | 1.27x | 10 |
| 3 | Java on HotSpot 25 | 78 ms | 1.30x | 10 |
| 4 | C with GCC 16 | 79 ms | 1.31x | 10 |
| 5 | Rust 1.99 | 92 ms | 1.54x | 10 |
| 6 | Python 3.14 | 119 ms | 1.99x | 10 |
| 7 | JavaScript on Node 24 | 154 ms | 2.57x | 10 |
| 8 | C# on .NET 10 JIT | 177 ms | 2.96x | 10 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/compress.cpp`](../../suite/cpp/src/compress.cpp) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| C | [`c.c`](c.c) | [`suite/c/src/work_compress.c`](../../suite/c/src/work_compress.c) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel unzstd

# 3. Full benchmark
python3 suite/run.py bench --kernel unzstd

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py unzstd --size 1048576 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
