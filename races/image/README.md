# Image resizing

Decodes 8 JPEG photos of 6,000 x 4,000 pixels, shrinks each to 1,080 wide and saves it at quality 80.

- **Workload:** 8 photos of 6,000 × 4,000
- **Reference machine:** Intel Core i7-13700F, Windows 11, one core, stock compiler/runtime settings
- **Verification:** every setup wrote 8 photos of 1,080 x 720
- **Harness sizes:** `--quick` = `1`, full = `8`
- **Expected checksum (`--quick`):** `1:1080x720`
- **Input data:** 8 synthetic 24 MP JPEG photos generated into `suite/data/photos/` (requires `python3 suite/run.py build --runtime py-work` once).

## Results

| Rank | Setup | Median | Relative | Warm repeats |
| ---: | :--- | ---: | ---: | ---: |
| 1 | JavaScript with sharp | 533 ms | 1.00x (fastest) | 10 |
| 2 | C# with SkiaSharp | 1,287 ms | 2.41x | 8 |
| 3 | C++ with stb_image | 1,342 ms | 2.51x | 8 |
| 4 | C with stb_image | 1,351 ms | 2.53x | 8 |
| 5 | Python with Pillow | 1,716 ms | 3.22x | 6 |
| 6 | Java with ImageIO | 2,005 ms | 3.76x | 5 |
| 7 | Rust with the image crate | 2,565 ms | 4.81x | 4 |
| 8 | Go with image/jpeg | 4,992 ms | 9.36x | 3 |

## Source code

> **Note:** Each file in this folder contains **only the code for this race**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. To build and run (with shared input generators and manifests), run from the repository root via `suite/run.py`.

| Language | Race file | Full module in `suite/` |
| :--- | :--- | :--- |
| JavaScript | [`js.mjs`](js.mjs) | [`suite/js/work.mjs`](../../suite/js/work.mjs) |
| C# | [`csharp.cs`](csharp.cs) | [`suite/csharp/Work.cs`](../../suite/csharp/Work.cs) |
| C++ | [`cpp.cpp`](cpp.cpp) | [`suite/cpp/src/image.cpp`](../../suite/cpp/src/image.cpp) |
| C | [`c.c`](c.c) | [`suite/c/src/work_image.c`](../../suite/c/src/work_image.c) |
| Python | [`python.py`](python.py) | [`suite/python/work.py`](../../suite/python/work.py) |
| Java | [`java.java`](java.java) | [`suite/java/src/main/java/kernels/Work.java`](../../suite/java/src/main/java/kernels/Work.java) |
| Rust | [`rust.rs`](rust.rs) | [`suite/rust/src/work.rs`](../../suite/rust/src/work.rs) |
| Go | [`go.go`](go.go) | [`suite/go/work.go`](../../suite/go/work.go) |

## How to run this race

```bash
# 1. Build the runtimes you want to test (see docs/SETUP.md for compiler env vars)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py-work

# 2. Quick verification against suite/expected.json
python3 suite/run.py bench --quick --kernel image

# 3. Full benchmark
python3 suite/run.py bench --kernel image

# 4. Or run one language directly (no harness)
cd suite/python && ../.venv-work/bin/python main.py image --size 1 --repeats 3 --data ../data
```

- Platform setup (Windows, macOS, Linux) and troubleshooting: [docs/SETUP.md](../../docs/SETUP.md)
- Timing rules, CPU pinning, and warm repeats: [docs/METHODOLOGY.md](../../docs/METHODOLOGY.md)
- Exact input generators and checksum contract: [SPEC.md](../../SPEC.md)
