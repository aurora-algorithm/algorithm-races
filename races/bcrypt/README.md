# Bcrypt

Hashes a password slowly on purpose, so every guess costs an attacker the same work.

- **Workload:** 32 passwords at cost 10
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `2`, full = `32`
- **Expected checksum (`--quick`):** `2:2:1`

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | C with GCC 16 | 1,178 ms | 1.00x (fastest) | 9 |
| 2 | Go 1.27 | 1,316 ms | 1.12x | 8 |
| 3 | C++ with GCC 16 | 1,325 ms | 1.12x | 8 |
| 4 | Rust 1.99 | 1,371 ms | 1.16x | 8 |
| 5 | Python 3.14 | 1,373 ms | 1.17x | 8 |
| 6 | Java on HotSpot 25 | 1,472 ms | 1.25x | 7 |
| 7 | JavaScript on Node 24 | 1,536 ms | 1.30x | 7 |
| 8 | C# on .NET 10 JIT | 1,558 ms | 1.32x | 5 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| C | [`c.c`](c.c) | [`suite/c/src/work_password.c`](../../suite/c/src/work_password.c) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/passwords.cpp`](../../suite/cpp/src/passwords.cpp) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel bcrypt

# 3. Full benchmark
python3 suite/run.py bench --kernel bcrypt

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py bcrypt --size 2 --repeats 3
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
