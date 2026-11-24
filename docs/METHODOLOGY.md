# Benchmark Methodology

Every published number in [`races/`](../races/) comes directly from `suite/run.py` measurements — no numbers are typed by hand or estimated. Full algorithmic definitions and input generators are specified in [SPEC.md](../SPEC.md).

## 1. Program contract & what is timed

Every language implements the same CLI contract:

```text
<program> KERNEL --size N [--repeats R] [--threads T] [--data DIR]
```

- **Outside the timer:** Input generation (`xorshift32` with seed `2463534242`), loading data files into memory (where specified), and cloning a fresh unsorted/unmodified input copy before each repeat.
- **Inside the timer:** Only the kernel call itself, measured with each language's monotonic clock (`clock_gettime` / `QueryPerformanceCounter`, `std::chrono::steady_clock`, `Instant::now`, `time.Now`, `Stopwatch`, `System.nanoTime`, `performance.now`, `time.perf_counter_ns`).
- **Memory allocation & freeing:** Included in the timed region whenever the algorithm allocates working memory (for example, `mergesort`'s auxiliary buffer or `binary-trees` node allocation/freeing, so manual-memory and GC languages pay for the same lifecycle).
- **Output:** One JSON line on stdout containing `kernel`, `size`, `threads`, `checksum`, `times_ms`, and `runtime`.

## 2. Cold vs warm repeats

For each `(kernel, threads, runtime)` cell, `suite/run.py` runs:
1. **Cold run (`--repeats 1`):** One fresh process measuring startup, wall time, CPU time, peak RSS, and first-call kernel time.
2. **Warm repeats:** One process running `R` repeats back to back, where `R` is chosen from the cold time to stay within a 10-second warm budget (clamped between 3 and 10 repeats; 2 repeats if a single cold call already exceeds 30 seconds).
3. **Reported metric:** The **median of warm repeats `2..R`** (discarding repeat 1 so JITs like HotSpot, .NET, and V8 are warmed up, and caches are hot across all languages).

## 3. CPU pinning & reference machine

- **Reference hardware:** Intel Core i7-13700F desktop, Windows 11, native execution (no WSL, no VM, no Docker).
- **Single-threaded kernels:** Whole process pinned to logical CPU 0 (one P-core, SMT sibling idle). Runtimes with background JIT/GC threads share that same core.
- **Multi-threaded (`-mt`) kernels:** Process pinned to `T` P-cores (`0, 2, ..., 2T-2`, skipping SMT siblings).
- **Linux:** Uses `os.sched_setaffinity` with the same CPU list.
- **macOS:** Runs unpinned (macOS does not expose strict per-core affinity).

## 4. Stock settings policy

Every compiler and runtime runs with **stock release settings**:
- **C / C++:** `-O3` (GCC / Clang) or `/O2` (MSVC); no `-march=native`, no LTO, no PGO, no `-ffast-math`.
- **Rust:** `cargo build --release` (stock `Cargo.toml`, no `target-cpu=native`).
- **Go:** `go build` (default flags).
- **Goose:** `goose build --cc msvc` / `--cc clang` (`--cc clang-cl` on Windows; default `-O2`).
- **C#:** `dotnet publish -c Release` (stock .NET 10 tiered JIT with default PGO).
- **Java:** HotSpot 25 default server JIT (`java -jar target/kernels.jar`).
- **JavaScript:** Stock Node 24 (`node main.mjs`).
- **Python:** Stock CPython 3.14 (`python3.14 main.py`).

Every language uses its natural idiomatic types and standard library unless a kernel explicitly tests a named library swap (e.g. `regex-log-alt`, `image-alt`, `csv-alt`).

## 5. Checksum verification

Every kernel computes a deterministic checksum (`h31`, `fnv1a`, or exact count/hash — see [SPEC.md](../SPEC.md)) after the last repeat and prints it in its JSON output. `suite/run.py` compares every cell against [`suite/expected.json`](../suite/expected.json) and flags any mismatch as `not ok`.

Exceptions where outputs intentionally differ by library (e.g. `gzip`/`zstd` compressed byte streams or JPEG encoder output) verify round-trip integrity and are noted in the corresponding race's README.

## 6. Output files

Each run of `suite/run.py bench` writes a session folder under `suite/results/<session>/`:
- `kernels.jsonl` — Raw per-round, per-cell JSON records (including every individual repeat in `times_ms`, wall/CPU time, peak RSS, and checksum).
- `summary.csv` — Aggregated table with median ms, first-repeat ms, process wall ms, peak RSS MB, relative slowdown `x`, and `checksum_ok`.
- `meta.json` — CPU model, OS, git commit, exact command-line flags, and full version string of every runtime that ran.
