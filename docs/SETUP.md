# Setup & Running Benchmarks (Windows, macOS, Linux)

The benchmark runner (`suite/run.py` and `suite/check.py`) uses **Python 3.11+ standard library only** — you do not need to install any Python packages just to run the harness.

You also do **not** need every language installed. `suite/run.py bench` automatically skips runtimes whose toolchain or binary is missing. For `suite/run.py build`, pass `--runtime <id>` for the runtimes you want to build (running bare `suite/run.py build` attempts every non-opt-in runtime in `suite/runtimes.json`, including Wasm, TinyGo, Nuitka, and Goose, and fails on any missing toolchain).

> **Important:** Each file inside `races/<slug>/` (`c.c`, `rust.rs`, `python.py`, ...) contains **only that race's code**, extracted verbatim from `suite/` so you can read every language side by side without unrelated algorithms. Always build and run from the repository root via `suite/run.py` (or inside `suite/<lang>/`).

---

## 1. Prerequisites & runtime IDs

| Runtime ID(s) | Language | Required toolchain | Required / optional env vars |
| :--- | :--- | :--- | :--- |
| `c-gcc`, `c-clang`, `c-msvc` | C | `gcc` or `clang` (or MSVC on Windows), `curl`, `tar`, `bzip2` | Full path in `CPP_GCC` / `CPP_CLANG` (plus `GCC` on Homebrew macOS); `CPP_VCVARS` on Windows |
| `cpp-gcc`, `cpp-clang`, `cpp-msvc` | C++ | C++23 compiler with `<print>` (`g++` 14+ or `clang++` 18+, or MSVC on Windows), `curl`, `tar`, `xz` | Full path in `CPP_GCC` / `CPP_CLANG`; `CPP_VCVARS` on Windows |
| `rust` | Rust | `rustc` + `cargo` (1.85+ / edition 2024) | — |
| `go` | Go | `go` (1.27.1+, or older Go with `GOTOOLCHAIN=auto`) | — |
| `goose-clang`, `goose-msvc`, `goose-tcc` | Goose | `goose` compiler (`clang` on macOS/Linux; `clang-cl` or MSVC on Windows) | `GOOSE`, `GOOSE_CLANG=1`, `GOOSE_MSVC=1` |
| `cs-jit`, `cs-aot` | C# | `.NET 10 SDK` (`dotnet`) — `Kernels.csproj` targets `net10.0` | — |
| `java` | Java | **JDK 25** + Maven (`mvn`) — `pom.xml` sets `<maven.compiler.release>25</maven.compiler.release>` | `JAVA_HOME` (required on Linux, Windows, and Intel macOS) |
| `node`, `bun`, `deno` | JavaScript | `node` + `npm` (for `npm ci`), and optionally `bun` / `deno` | — |
| `py314`, `py313`, `py312` | Python | `python3.14` (or `python3.13` / `python3.12`) | `PY314`, `PY313`, `PY312` |
| `py-work`, `py-libs` | Python (work & builtins) | `python3.14` + `uv` (creates `suite/.venv-work` / `suite/.venv-builtins`) | `PYWORK`, `PYLIBS` |

Notice that `c-msvc`, `cpp-msvc`, and `goose-msvc` are **Windows-only**. All other runtimes build and run on macOS, Linux, and Windows.

---

## 2. Platform-specific setup

### macOS (Apple Silicon & Intel)

1. Install Xcode Command Line Tools (`clang`, `git`, `curl`, `tar`):
   ```bash
   xcode-select --install
   ```
2. Install the languages you want via [Homebrew](https://brew.sh/):
   ```bash
   brew install gcc rust go dotnet openjdk maven node python@3.14 uv
   ```
3. Export **full paths** for C/C++ compilers and JDK so `suite/c/build.sh` and `suite/cpp/build.sh` resolve them cleanly:
   ```bash
   export CPP_GCC="$(which g++-16 2>/dev/null || which g++-15 2>/dev/null || which g++)"
   export GCC="$(which gcc-16 2>/dev/null || which gcc-15 2>/dev/null || which gcc)"
   export CPP_CLANG="$(which clang++)"
   export JAVA_HOME="$(brew --prefix openjdk)"
   ```
   *(Homebrew installs versioned `gcc-16`/`g++-16`; exporting `GCC` lets `suite/c/build.sh` find `gcc-16` even though `dirname "$CPP_GCC"` contains `gcc-16` instead of unversioned `gcc`.)*
4. Build and verify:
   ```bash
   python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314
   python3 suite/run.py bench --quick --kernel mergesort --runtime rust --runtime go --runtime node --runtime py314
   ```
5. **macOS pinning note:** macOS does not support strict per-core process affinity (`sched_setaffinity`), so `suite/run.py` runs unpinned on macOS and records that in `meta.json`.

### Linux (Ubuntu / Debian / Fedora / Arch)

1. Install base build tools + decompression utilities (`bzip2` for Mbed TLS, `xz-utils` for Botan) + the languages you want (Ubuntu/Debian example):
   ```bash
   sudo apt update
   sudo apt install -y build-essential clang curl tar bzip2 xz-utils git maven nodejs npm python3
   ```
   Install newer toolchains as needed:
   - **Rust (1.85+ / edition 2024):** `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
   - **Go (1.27.1+):** https://go.dev/dl/
   - **.NET 10 SDK:** https://dotnet.microsoft.com/download/dotnet/10.0
   - **JDK 25:** https://adoptium.net/ or `sdkman`
   - **Python 3.14 / `uv`:** `curl -LsSf https://astral.sh/uv/install.sh | sh && uv python install 3.14`
2. Export **full paths** for compiler and JDK environment variables:
   ```bash
   export CPP_GCC="$(which g++)"
   export CPP_CLANG="$(which clang++)"
   export JAVA_HOME="/usr/lib/jvm/temurin-25-jdk-amd64"   # required; adjust to your JDK 25 path
   ```
3. Build and verify:
   ```bash
   python3 suite/run.py build --runtime c-gcc --runtime cpp-gcc --runtime rust --runtime go --runtime node --runtime py314
   python3 suite/run.py bench --quick --kernel mergesort
   ```
4. **Linux pinning:** Single-threaded kernels are automatically pinned to CPU 0 (`os.sched_setaffinity`), and `-mt` kernels to CPUs `0, 2, ..., 2T-2`.

### Windows 11 (native — not WSL)

1. Install **Git for Windows** (provides Git Bash). `suite/run.py` invokes Git Bash (`C:/Program Files/Git/bin/bash.exe` or `C:/Program Files/Git/usr/bin/bash.exe`) and never uses WSL's `C:/Windows/System32/bash.exe`. If Git is installed elsewhere, set `BASH_EXE`.
2. Open **Git Bash** and install the toolchains you want (e.g. via `winget`, `rustup`, `dotnet`, Adoptium JDK 25, Node.js, Python 3.14). Note: on Windows, use `python` (or `py -3`) if `python3` opens the Microsoft Store stub.
3. Set environment variables for C/C++/Java if testing them:
   ```bash
   # MSVC (Visual Studio 2022 Community/Build Tools — also needed by c-clang/cpp-clang on Windows)
   export CPP_VCVARS="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"

   # Optional: full paths to GCC (WinLibs MinGW-w64 UCRT) and Clang
   export CPP_GCC="$(which g++)"
   export CPP_CLANG="$(which clang++)"

   # Java 25 (required)
   export JAVA_HOME="C:/Program Files/Eclipse Adoptium/jdk-25"
   ```
4. Build and verify:
   ```bash
   python suite/run.py build --runtime rust --runtime go --runtime cs-jit --runtime node --runtime py314
   python suite/run.py bench --quick --kernel mergesort
   ```
5. **Windows pinning:** Processes are automatically pinned to CPU 0 (or `0, 2, ..., 2T-2` for `-mt`) via Win32 `SetProcessAffinityMask`.

---

## 3. Common commands

### Build
```bash
# Always pass --runtime for the languages you have installed
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# For real-world workloads (image, csv, gzip, zstd, bcrypt, argon2id, wordcount), also build py-work:
python3 suite/run.py build --runtime py-work
```
*Note:* Building `c-*` or `cpp-*` automatically runs `suite/c/fetch_deps.sh` / `suite/cpp/deps.sh` on first run to fetch pinned C/C++ libraries (`zlib`, `zstd`, `PCRE2`, `Botan`, etc.) into `suite/c/deps/` and `suite/cpp/third_party/` (both gitignored). Running bare `python3 suite/run.py build` without `--runtime` attempts every non-opt-in runtime in `suite/runtimes.json` (including Wasm, TinyGo, Nuitka, and Goose).

### Benchmark & verify checksums
```bash
# Quick size (~milliseconds per kernel) + verify against suite/expected.json
python3 suite/run.py bench --quick --kernel bfs-grid

# Probe size (~1/10 of full work) to estimate full-run time
python3 suite/run.py bench --probe --kernel bfs-grid --session my-probe
python3 suite/run.py estimate suite/results/my-probe

# Full size (the exact workload published in races/<slug>/README.md)
python3 suite/run.py bench --kernel bfs-grid --rounds 3 --session my-run

# Benchmark an entire group: sort, algo, puzzle, bg, lib, loop, mt, work, builtins
python3 suite/run.py bench --quick --group sort
```

### Run one language directly (no harness)
Every language compiles to a single CLI binary (or script) with the exact same interface:
```text
<program> KERNEL --size N [--repeats R] [--threads T] [--data DIR]
```
Examples:
```bash
# Python (zero build step for non-work kernels)
cd suite/python && python3 main.py mergesort --size 20000 --repeats 3

# Rust
cd suite/rust && cargo build --release && ./target/release/kernels mergesort --size 10000000 --repeats 5

# Go
cd suite/go && go build -o bin/kernels . && ./bin/kernels mergesort --size 10000000 --repeats 5

# Node.js
cd suite/js && npm ci && node main.mjs mergesort --size 20000 --repeats 3
```
Verify every quick-size checksum for one program at once:
```bash
python3 suite/check.py --cwd suite/rust --lang rust -- target/release/kernels
python3 suite/check.py --cwd suite/python --lang python --kernel mergesort -- python3 main.py
```

---

## 4. Real-world workload data (`work` group)

The `json`, `image`, `image-alt`, `csv`, `csv-alt`, `gzip`, `gunzip`, `zstd`, `unzstd`, and `wordcount` kernels read generated files from `suite/data/` (gitignored):
- `suite/run.py` and `suite/check.py` generate `orders-*.json`, the 100 MB mixed corpus, the 1 GB orders CSV, and the 1 GiB text file automatically on first use.
- **Python `work` kernels (`py-work`):** `py314` skips the `work` group (`skip_groups: ["work"]` in `suite/runtimes.json`). Build `py-work` (which uses `uv` to create `suite/.venv-work` with Pillow, zstandard, argon2-cffi, bcrypt, etc.):
  ```bash
  python3 suite/run.py build --runtime py-work
  ```
- **Photos (`image`, `image-alt`):** Generating the 8 synthetic 24 MP JPEG photos requires Pillow from `py-work`, so run `python3 suite/run.py build --runtime py-work` once before benchmarking `image` in any language.
- When invoking a language binary directly on a `work` or `json` kernel, pass `--data ../data` (after `suite/data/` has been populated), and for Python use `../.venv-work/bin/python` (`../.venv-work/Scripts/python.exe` on Windows).

---

## 5. Troubleshooting

- **`./gcc: No such file or directory` when building `c-gcc` / `c-clang`:** `CPP_GCC` and `CPP_CLANG` must be **full paths** (`export CPP_GCC="$(which g++)"`, not bare `"g++"`). On macOS Homebrew, also export `GCC="$(which gcc-16)"`.
- **`skip c-gcc: $CPP_GCC not set` (or `CPP_CLANG`, `CPP_VCVARS`):** Export the compiler variable before running `build` and `bench`.
- **`java` build fails with `release version 25 not supported` or `java` is skipped:** `suite/java/pom.xml` targets JDK 25 and `suite/runtimes.json` requires `JAVA_HOME` (outside Apple Silicon Homebrew).
- **`csharp` build fails with `NETSDK1045`:** `suite/csharp/Kernels.csproj` targets `net10.0`. Install the .NET 10 SDK.
- **`py314` skipped (`python3.14 not on PATH`):** Set `export PY314="$(which python3)"` (or install Python 3.14 and point `PY314` to it).
- **`Git Bash not found` on Windows:** Install Git for Windows or set `BASH_EXE` to your `bash.exe` path (never WSL).
- **Noisy numbers:** Close browsers/IDEs, plug in laptops, and pass `--rounds 3` so `summary.csv` takes the median across multiple independent rounds.
