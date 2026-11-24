# Photo shrink with other libraries

Shrinks 8 large JPEG photos with each language's other well-known image library.

- **Workload:** 8 photos of 6,000 × 4,000 pixels
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup returned the same checksum
- **Harness sizes:** `--quick` = `1`, full = `8`
- **Expected checksum (`--quick`):** `1:1080x720`
- **Input data:** 8 synthetic 24 MP JPEG photos generated into `suite/data/photos/` (requires `python3 suite/run.py build --runtime py-work` once).

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | Python with pyvips | 631 ms | 1.00x (fastest) | 10 |
| 2 | C# with NetVips | 635 ms | 1.01x | 10 |
| 3 | Rust with fast_image_resize | 883 ms | 1.40x | 10 |
| 4 | Java with Thumbnailator | 2,958 ms | 4.69x | 3 |
| 5 | Go with imaging | 4,533 ms | 7.18x | 3 |
| 6 | JavaScript with Node · jimp | 20,248 ms | 32.1x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel image-alt

# 3. Full benchmark
python3 suite/run.py bench --kernel image-alt

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py image-alt --size 1 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
