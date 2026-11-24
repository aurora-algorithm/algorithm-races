# Argon2id password hashing

Hashes 32 passwords with Argon2id at its recommended setting, 19 MiB of memory and two passes each.

- **Workload:** 32 passwords at 19 MiB each
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `2`, full = `32`
- **Expected checksum (`--quick`):** `2:2:1`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Rust 1.99 | 401 ms | 1.00x (fastest) | 10 |
| 2 | Go 1.27 | 413 ms | 1.03x | 10 |
| 3 | C++ with GCC 16 | 455 ms | 1.13x | 10 |
| 4 | C with GCC 16 | 489 ms | 1.22x | 10 |
| 5 | Python 3.14 | 507 ms | 1.26x | 10 |
| 6 | JavaScript on Node 24 | 510 ms | 1.27x | 10 |
| 7 | Java on HotSpot 25 | 553 ms | 1.38x | 10 |
| 8 | C# on .NET 10 JIT | 1,050 ms | 2.62x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/passwords.cpp`](../../suite/cpp/src/passwords.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/work_password.c`](../../suite/c/src/work_password.c) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel argon2id

# 3. Full benchmark
python3 suite/run.py bench --kernel argon2id

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py argon2id --size 2 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
