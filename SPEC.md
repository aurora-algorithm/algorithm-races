# kernels: CPU benchmarks across six languages

A second track beside the HTTP benchmark: well-known CPU kernels (sorting, the Benchmarks Game, library calls) and
everyday work with each language's usual libraries (resize photos, compress, hash passwords, parse CSV, count words;
section 11), implemented the same way in Rust, Go, C#, Java, JavaScript and Python, measured on the i7-13700F (native
Windows 11).
Every kernel is (a) a fair, idiomatic, well-known task, (b) easy to show on screen,
(c) checkable: every language prints the same checksum, (d) honest: stock settings, and what was measured is written
down. Every number in a video traces back to a row in `results/<session>/`.

`kernels/python/` is the reference: its quick-size checksums are `expected.json`. When this file and the Python code
disagree, fix one of them; the code shown in a video must match this text.

## 1. Program contract

One program per language with every kernel in it:

```
<program> KERNEL --size N [--repeats R] [--threads T] [--data DIR]
```

- Builds its input from `--size` (section 2) before timing; `--data` is the folder holding `orders-<N>.json`.
- Runs the kernel R times (default 1) in one process. Each repeat gets a fresh copy of the input (a sort sorts an
  unsorted copy every time); the copy is made outside the timed region. Only the kernel call is timed, with the
  language's monotonic clock (`Instant`, `time.Now`, `Stopwatch`, `System.nanoTime`, `performance.now`,
  `time.perf_counter_ns`).
- Prints exactly one line to stdout, JSON, keys in this order:
  `{"kernel":"sort-int","size":10000000,"threads":1,"checksum":"240352578","times_ms":[812.4,790.1],"runtime":"..."}`
  `checksum` is a string, computed from the last repeat's result after timing. `times_ms` has one number per repeat
  (milliseconds, 3 decimals is plenty). `runtime` is free text naming the runtime as the program sees it (version,
  GC, JIT/AOT; for the work kernels also the libraries and their versions). An optional last key `extra` holds
  numbers that differ by library and are not part of the checksum (`{"out_bytes": 812345}`, section 11).
- Exit code 0; unknown kernel -> exit 2. Nothing else on stdout. Stack size: the default (no kernel recurses deeper
  than about 50 levels). Freeing is part of the kernel where it happens inside it (Rust drops each tree inside the
  timed region, as the GC languages collect theirs).
- Single-threaded kernels use one thread for the work (runtimes may still run their own JIT/GC threads; the runner
  pins the whole process to one CPU, so those share it).

## 2. Shared inputs and checksums

**Generator**: xorshift32 (Marsaglia), 32-bit unsigned state, seed `2463534242`, fresh generator per input:

```
s ^= s << 13   (keep 32 bits)
s ^= s >> 17
s ^= s << 5    (keep 32 bits)
return s       (values 1 .. 2^32-1)
```

The first three values from the seed are `723471715`, `2497366906`, `2064144800`.

**Integers** are stored in each language's natural 64-bit integer array (`Vec<i64>`, `[]int64`, `long[]`, `long[]`,
JavaScript numbers, Python ints). The values fit in 32 bits unsigned, so JavaScript numbers hold them exactly.

**Index and count arrays** take each language's natural integer, so the same array can be twice as wide in one
language as in another (more memory traffic for the same algorithm): Rust `usize` and Go `int` are 64-bit (e.g.
dijkstra's `to`, kruskal's `parent`, the sudoku links, palindrome's `p`, bwt's `sa`), Goose's default `i64` is
64-bit (every Goose array except the sudoku links and the collatz cache, which are `i32`), C# and Java `int[]` are
32-bit, JS uses numbers or `Int32Array`, Python ints. The collatz cache and the -arena links are 32-bit in every
language.

**h31(values)**: `h = 0; for v: h = (h * 31 + v) mod 2^32`, printed as an unsigned decimal.

**fnv1a(bytes)**: FNV-1a 32-bit, `h = 2166136261; for b: h = ((h ^ b) * 16777619) mod 2^32`, unsigned decimal.

| input | definition |
|---|---|
| ints(N) | the first N generator values |
| strings(N) | for each string: `len = 8 + next() % 9`, then `len` characters `'a' + next() % 26` |
| sha buffer(N bytes) | one 1 MiB block = 262,144 generator values, each written as 4 bytes little-endian; the block repeated N / 1 MiB times (N is a multiple of 1 MiB) |
| log(N lines) | per line, in this order: `method = ["GET","POST","PUT","DELETE"][next() % 4]`, `v = 1 + next() % 3`, `user = next() % 100000`, `status = [200,201,204,301,304,400,401,403,404,500,502,503][next() % 12]`, `ms = next() % 1000`; line = `"{method} /api/v{v}/users/{user} {status} {ms}ms\n"`; all lines concatenated into one string |
| matrices(n) | A = the first n*n values `% 10` as doubles, row-major; B = the next n*n values the same way (one generator for both) |
| hashmap(N) | `ins` = ints(N); `look[i] = ins[i] xor (i and 1)` (even i: a key that is in the map; odd i: its neighbour, usually not) |
| orders(N) | the file `kernels/data/orders-<N>.json` written by `run.py` (from `python/gen.py orders_json`): `{"orders":[{"id":i,"customer_id":(i-1)/20+1,"status":S[i%4],"total_cents":500+(i*7919)%50000,"items":1+(i*31)%9,"tags":["a"+(i%7),"b"+(i%11)]},...]}` for i = 1..N, S = paid, shipped, delivered, refunded, compact |

## 3. Sorting kernels (same input: ints(N); checksum h31 of the sorted array, the same for all six)

- `sort-int`: the language's standard sort on its primitive array, ascending, as-is: Rust `slice::sort` (stable),
  Go `slices.Sort`, C# `Array.Sort(long[])`, Java `Arrays.sort(long[])`, JavaScript `array.sort((a, b) => a - b)` on
  a plain Array of numbers, Python `list.sort()`. (Each picks its own algorithm; that is the point of this row.)
- `sort-str`: the standard sort on strings(N), ordinal order (byte / code-unit order; all strings are ASCII, so every
  language's ordinal order agrees). C# uses `StringComparer.Ordinal` (its default comparer is culture-aware, which
  would be a different, slower comparison). Checksum: fnv1a of the sorted strings, each followed by `\n`.
- `quicksort`: Hoare partition, pivot = the middle element, recurse into the smaller side and loop on the larger:

```
quicksort(a, lo, hi):            # inclusive bounds; called as quicksort(a, 0, n-1)
  while lo < hi:
    p = a[lo + (hi - lo) / 2]
    i = lo - 1; j = hi + 1
    loop:
      do i += 1 while a[i] < p
      do j -= 1 while a[j] > p
      if i >= j: break
      swap a[i], a[j]
    if j - lo < hi - j: quicksort(a, lo, j); lo = j + 1
    else:               quicksort(a, j + 1, hi); hi = j
```

- `mergesort`: top-down, one aux array of n allocated per call (inside the timed region), stable:

```
sort(a, aux, lo, hi):            # half-open [lo, hi)
  if hi - lo < 2: return
  mid = lo + (hi - lo) / 2
  sort(a, aux, lo, mid); sort(a, aux, mid, hi); merge(a, aux, lo, mid, hi)
merge(a, aux, lo, mid, hi):
  copy a[lo..hi) into aux[lo..hi)  (the language's block copy)
  i = lo; j = mid; k = lo
  while i < mid and j < hi: if aux[j] < aux[i]: a[k++] = aux[j++] else: a[k++] = aux[i++]
  copy the rest of the left run, then the rest of the right run
```

- `heapsort`: max-heap, sift-down, build from n/2-1 down to 0, then n-1 down to 1: swap a[0], a[end]; sift_down(0, end):

```
sift_down(a, root, end):         # end exclusive
  while 2*root + 1 < end:
    child = 2*root + 1
    if child + 1 < end and a[child] < a[child+1]: child += 1
    if a[root] < a[child]: swap a[root], a[child]; root = child
    else: return
```

- `radixsort`: LSD, 8 bits per pass, 4 passes (shift 0, 8, 16, 24), counting sort into a second array of n
  (allocated per call), then the arrays swap roles; after 4 passes the result is back in the first.

Counts of comparisons, swaps and writes for the animation come from `trace/sort_trace.py` (one implementation; the
algorithm and the input are the same in every language, so are the counts).

## 4. Benchmarks Game kernels (single-threaded; their rules where they exist)

Floating-point code keeps the operation order written here (and in `python/bg.py`), no fused multiply-add, so the
printed results match to the last digit. (Go fuses `x*y + z` on arm64; on the i7 (amd64, GOAMD64=v1) it does not.)

- `binary-trees` N: min_depth 4, max_depth = max(6, N). A node is an object/struct with two child references,
  allocated one by one with the language's normal allocation (Rust `Box`, Go `&Node{}`, a class in C#/Java/JS, a
  tuple in Python as in its Benchmarks Game program; no arenas, no pools). `make(d)`: d == 0 -> a leaf (no
  children), else a node with `make(d-1)` twice. `check(t)` = 1 for a leaf, else `1 + check(left) + check(right)`.
  Output text (then checksum = fnv1a of it, UTF-8, `\n` after every line, `\t` as written):
  ```
  stretch tree of depth {max+1}\t check: {check(make(max+1))}
  for d = 4, 6, .. max: {iterations}\t trees of depth {d}\t check: {sum of check(make(d)) over iterations}
                        where iterations = 1 << (max - d + 4)
  long lived tree of depth {max}\t check: {check(long_lived)}    (long_lived = make(max), made before the loop)
  ```
  Goose has no heap, so it runs only `binary-trees-arena` (section 12, "Arena rows"), the same text and checksum.
- `nbody` N steps: the five bodies, constants and `offset_momentum` of the Benchmarks Game (`python/bg.py`
  `BODIES`; PI = 3.141592653589793, SOLAR_MASS = 4*PI*PI, DAYS_PER_YEAR = 365.24, velocities are given values *
  DAYS_PER_YEAR, masses * SOLAR_MASS). dt = 0.01. One step, pairs (i, j) for i < j in body order:
  ```
  dx = xi - xj; dy = yi - yj; dz = zi - zj
  d2 = dx*dx + dy*dy + dz*dz
  mag = dt / (d2 * sqrt(d2))
  mi = mass_i * mag; mj = mass_j * mag
  vxi -= dx * mj; vyi -= dy * mj; vzi -= dz * mj
  vxj += dx * mi; vyj += dy * mi; vzj += dz * mi
  then for every body: x += dt * vx; y += dt * vy; z += dt * vz
  ```
  energy: `e += 0.5 * m * (vx*vx + vy*vy + vz*vz)` per body (left to right: `(0.5*m) * (...)`), then for each pair
  `e -= (mi * mj) / sqrt(dx*dx + dy*dy + dz*dz)`, inside the same body loop (body i, then its pairs). Checksum:
  `"{energy before:.9f};{energy after:.9f}"` (fixed 9 decimals, `-` sign, `.` decimal point).
- `mandelbrot` N: N x N, 50 iterations, escape when `tr + ti > 4.0`:
  ```
  ci = 2.0*y/N - 1.0;  cr = 2.0*x/N - 1.5        ((2.0*y)/N, then - 1.0)
  zr = zi = tr = ti = 0; i = 0
  while i < 50 and tr + ti <= 4.0: zi = 2.0*zr*zi + ci; zr = tr - ti + cr; tr = zr*zr; ti = zi*zi; i++
  in the set if tr + ti <= 4.0
  ```
  Bits packed MSB first, each row padded to whole bytes ((N+7)/8 bytes per row), rows concatenated (the PBM body,
  no header). Checksum: fnv1a of those bytes.
- `fannkuch` N: the Benchmarks Game's sequential algorithm (`python/bg.py fannkuch`: perm1/count arrays, the
  rotation order, alternating sign of the checksum). Checksum `"{checksum}:{maxflips}"`.
- `spectral-norm` N: `A(i,j) = 1.0 / ((i+j)*(i+j+1)/2 + i + 1)` (integer arithmetic in the denominator, 64-bit
  safe, then converted); u = 1.0 repeated N; 10 times: v = AtA(u), u = AtA(v), where `Av[i] = sum_j A(i,j)*u[j]` and
  `Atv[i] = sum_j A(j,i)*u[j]` summed left to right in a plain loop (Python's `sum()` would use compensated summation,
  so the reference loops too). Result `sqrt(sum(u[i]*v[i]) / sum(v[i]*v[i]))`, checksum `"{:.9f}"`.

## 5. Library and loop kernels

- `regex-log` N lines: on log(N), find all non-overlapping matches of `(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])`
  over the whole string with the standard regex engine, compiled once before the timed region, as constructed by default (Rust `regex` crate, Go `regexp`,
  C# `new Regex(p)` interpreted, Java `Pattern.compile`, JS `/.../g` with `matchAll`, Python `re`). Checksum
  `"{matches}:{sum of group 2 as integers}"`. The engines are not the same kind, so this row compares engines:

  | row | engine |
  |---|---|
  | Rust (and the wasm rows) | `regex` crate: automata (lazy DFA, one pass per byte), no backtracking |
  | Go, go-tinygo | `regexp` (RE2 design): NFA / one-pass automata, no backtracking |
  | C# | `System.Text.RegularExpressions` interpreter (backtracking); C# builds the `Regex` before timing |
  | Java (all JVMs) | `java.util.regex` (backtracking); the `Pattern` is compiled before timing |
  | node, deno, graalnode | V8 Irregexp: backtracking, compiled to machine code |
  | bun | JavaScriptCore YARR: backtracking, JIT-compiled |
  | llrt, quickjs-ng, txiki | QuickJS `libregexp`: backtracking bytecode interpreter |
  | graaljs | TRegex: automaton-based, compiled by Truffle |
  | CPython rows | `re` (sre, backtracking C); PyPy runs its own sre in RPython, JIT-compiled |
  | Goose | stdlib `regex`: DFAs built at compile time, a backtracker only for the capture groups inside a match |
  | C | PCRE2 10.49 (backtracking, its JIT off: the default build) |
  | C++ | `std::regex` of each compiler's STL (backtracking); CTRE in `regex-log-alt` |

- `hashmap` N: the standard hash map with integer keys, default hasher and capacity: for k in ins: `m[k] += 1`
  (insert 1 if absent); then count the k in look that are keys. Rust `HashMap<i64, i64>` (SipHash, the default),
  Go `map[int64]int64`, C# `Dictionary<long, long>`, Java `HashMap<Long, Long>` (boxed: the standard map has no
  primitive variant), JS `Map`, Python `dict`. Checksum `"{len(m)}:{hits}"`.
- `sha256` N bytes: SHA-256 of the sha buffer (in memory) with the standard or de facto library: Rust `sha2`, Go
  `crypto/sha256`, C# `SHA256.HashData`, Java `MessageDigest`, JS `node:crypto` (all three runtimes), Python
  `hashlib`. These are native code (SHA-NI on the i7) in most of them: the row compares libraries, not languages.
  Rows without SHA-NI, so expect them to be several times slower: the wasm rows (`sha2` compiled to WebAssembly has
  no SHA instructions), `go-tinygo` (TinyGo builds `crypto/sha256`'s pure-Go code, not its assembly) and C (Mbed TLS
  is portable C; its sha256.c accelerates only Armv8). "All three runtimes" means node, bun and deno (and graalnode,
  llrt and node-portable, which have `node:crypto`); quickjs-ng, txiki and graaljs have no `node:crypto` and do not
  run sha256. Checksum: lowercase hex digest.
- `sieve` N: Eratosthenes on a byte (or bool) array of N+1, explicit loops:
  `for i = 2; i*i <= N: if !composite[i]: for j = i*i; j <= N; j += i: composite[j] = 1`, then count the primes in
  2..N. Checksum: the count.
- `matmul` n: C = A x B, naive i-j-k on flat row-major double arrays, `s = 0.0; for k: s += A[i*n+k] * B[k*n+j];
  C[i*n+j] = s`. All values are small integers, so every sum is exact. Checksum `"{sum of C as integer}:{h31(C)}"`.
- `json` N orders: read `orders-<N>.json` into a string before timing; timed: parse it fully, sum `total_cents`
  and `items` over the orders, serialise the whole document back to compact JSON (same key order, no whitespace).
  Typed where that is the language's norm: Rust serde structs, Go structs + `encoding/json`, C# records +
  System.Text.Json (source-generated, so the same code runs JIT and NativeAOT), Java records + Jackson databind
  (Java has no standard JSON library); JS `JSON.parse`/`JSON.stringify`, Python `json.loads`/`json.dumps` with
  `separators=(",", ":")`. The output must equal the input byte for byte. Checksum
  `"{orders}:{sum total_cents}:{sum items}:{output length in bytes}:{fnv1a(output)}"`.
  Typed (into structs or records) vs DOM (into the library's generic tree of objects): typed in Rust (serde, 64-bit
  fields), Go (structs, `int64`), C# (records, `long`; parsed from the file's UTF-8 bytes like Rust and Go, not from a
  UTF-16 string), Java (records, `long` ids and cents, `int` items), Goose (typed records from its `json` reader);
  DOM in JS (every engine), Python (every runtime: dicts and lists), C (cJSON) and C++ (nlohmann `ordered_json`). A
  DOM parse builds an object per value, so the two groups do different amounts of work for the same document.

## 6. Multi-threaded kernels (`--threads T`, T = 1, 2, 4, 8)

Threads (or JS `worker_threads`, Go goroutines) created before the timed region where starting them costs more
than about a millisecond (thread pools in Java, C#, Python; workers in JS); Rust uses `std::thread::scope` inside.
The work split is the same everywhere:

- `mergesort-mt` N (T a power of two): chunk t = `[t*N/T, (t+1)*N/T)`; T threads each run the sequential `mergesort` on their chunk
  (one shared aux array, disjoint ranges); then rounds of pairwise merges of neighbouring runs (each merge on its own
  thread) until one run is left (`python/mt.py`). Checksum as `sort-int`. JS: the array lives in a
  SharedArrayBuffer (Float64Array) shared by the workers.
- `mandelbrot-mt` N: thread t computes rows y = t, t+T, t+2T, ... into the shared output. Checksum as `mandelbrot`.
- `binary-trees-mt` N: stretch and long-lived trees on the main thread; for each depth the iterations are split
  into T equal shares (iterations is a power of two >= 16), each thread builds and checks its share, the main thread
  sums. Output and checksum as `binary-trees`.

Python runs these on a `ThreadPoolExecutor`: with the GIL the threads take turns (3.12, 3.13, 3.14, PyPy); 3.14t
runs them in parallel. That contrast is a result, not a bug.

## 7. Sizes

| kernel | full | quick | why the full size |
|---|---|---|---|
| sort-int, quicksort, mergesort, heapsort, radixsort, mergesort-mt | 10,000,000 | 20,000 | the owner's 10M |
| sort-str | 1,000,000 | 20,000 | |
| binary-trees, binary-trees-mt, binary-trees-arena | 21 | 10 | Benchmarks Game size |
| nbody | 5,000,000 | 1,000 | BG uses 50M; CPython would take ~3 min per run |
| mandelbrot, mandelbrot-mt | 4,000 | 200 | BG uses 16,000; CPython ~10 min per run |
| fannkuch | 11 | 7 | BG uses 12 (~13 min in CPython) |
| spectral-norm | 3,000 | 100 | BG uses 5,500 |
| regex-log | 2,000,000 lines (~70 MB) | 20,000 | |
| hashmap | 10,000,000 | 100,000 | |
| sha256 | 1 GiB | 4 MiB | |
| sieve | 100,000,000 | 1,000,000 | |
| matmul | 1,024 | 64 | |
| json | 450,000 orders (~45 MB) | 2,000 | |

The sizes are one constant across languages (`kernels.json`); a smaller BG size was picked where CPython would
otherwise dominate the session. The work kernels' sizes are in section 11.

## 8. Runtimes (`runtimes.json`)

Rust (release profile, stock: no LTO, no target-cpu), Go (stock), C# .NET 10 JIT (`dotnet publish -c Release`,
server GC off: a console app defaults to workstation GC) and NativeAOT, Java 25 HotSpot (Temurin, `java -jar`, no
flags), GraalVM CE 25 JIT and native-image (same jar), Node 24, Bun, Deno (the same ES modules, `node:` imports only),
CPython 3.12, 3.13, 3.14, 3.14t (free-threaded), PyPy 3.11, and `py-numpy` (CPython 3.14 + NumPy, only `sort-int`
and `matmul`, labelled as a C library: NumPy sorts with C/SIMD code and multiplies with BLAS, on
ndarrays built from the lists before timing (only `A @ B` is timed), one thread forced via
`OPENBLAS_NUM_THREADS=1`), and `py-work` (CPython 3.14 in `kernels/.venv-work` with the pinned
`python/requirements-work.txt`; runs only the work group, `"groups": ["work"]`). The other Python runtimes and
`java-native` skip the work group (`"skip_groups"`): the libraries are installed once, in one venv, and native-image
would need reachability metadata for AWT, ImageIO and JNI (Main looks the work kernels up by a run-time class name,
so the native binary of the other kernels is unchanged). The JS libraries come from `js/package-lock.json`
(`npm ci`); Bun and Deno load the same `node_modules`.

Compiler optimisation levels, all each toolchain's own release default: Rust `opt-level = 3` (the release
profile, LLVM's `-O3` pipeline), C and C++ with GCC and Clang `-O3 -DNDEBUG` (what CMake's Release build uses, and
the same LLVM level as Rust; chosen 2026-11-19 so C and C++ are not held back), MSVC `/O2 /DNDEBUG` (its highest
level), Go its one optimising mode, TinyGo its default `-opt=z`, Goose its own build defaults (`/O2` with MSVC).
zstd is built without its hand-written x86 assembly in C and C++ (`ZSTD_DISABLE_ASM`), as Rust's zstd-sys does on
Windows, so no row gets a decoder the others lack.

CPU features: the JIT runtimes (HotSpot, GraalVM JIT, OpenJ9, .NET JIT, V8, JavaScriptCore, PyPy, Numba, the CPython
JIT) compile for the CPU they run on, so on the i7 they use AVX2, BMI1/2 and the other x86-64-v3 instructions. The
ahead-of-time rows target a fixed baseline chosen at build time, not the i7: Rust (no `target-cpu`), Go
(`GOAMD64=v1`), TinyGo, C, C++, Goose, Nuitka, mypyc and Cython compile for baseline x86-64 (SSE2); .NET NativeAOT
and ReadyToRun use .NET's default ahead-of-time instruction set (no `IlcInstructionSet`), which stops below AVX2
(ReadyToRun methods that get hot are rejitted for the host); the wasm rows run WebAssembly, which has no AVX2
instructions. Java native-image is the exception: its default `-march` is x86-64-v3, so it does use AVX2. Libraries
that pick their code at run time (libvips, zlib-ng, libzstd, the SHA-NI paths of the crypto libraries, NumPy) use the
host's instructions in every row.

Garbage collectors, stock: HotSpot (Temurin and the other JDKs) picks Serial GC when it sees one CPU and G1 with two
or more (the runner pins single-threaded cells to one CPU, so Java's T=1 rows run Serial GC and its T=2..8 rows G1;
`runtime` names the collector); OpenJ9 uses its default `gencon` either way; `java-native` (GraalVM CE native-image)
has only the Serial GC (G1 is Oracle GraalVM only), at every thread count.

`go-tinygo` skips the mt group: on Windows TinyGo runs every goroutine on one OS thread (its `tasks` scheduler), so
a T = 8 row would time one core (`skip_why` in runtimes.json).

Settings a commenter could contest, all stock and on purpose: the JVM on one CPU picks Serial GC; .NET console apps
use workstation GC, so C# `binary-trees-mt` hardly scales (server GC would; not our default); .NET and Go size
themselves from the affinity mask; Rust's `HashMap` uses SipHash; C#'s regex is the interpreter (not
`RegexOptions.Compiled` or the source generator); JS sorts with a comparator on a plain Array (not a typed array); C#'s hash map update is
`m[k] = m.GetValueOrDefault(k) + 1` (two lookups; `CollectionsMarshal.GetValueRefOrAddDefault` would be one).

## 9. Measurements (`run.py`)

Per (kernel, runtime, threads): a **cold** process (repeats 1): whole-process wall time (spawn to exit: startup,
input generation, kernel, checksum), CPU time (user + system), peak RSS, the kernel time; then a **warm** process
with R repeats, R = clamp(ceil(10 s / cold kernel time), 3, 10), or 2 when the cold kernel took over 30 s: first
repeat, median and min of repeats 2..R. The warm number every table and video uses is that median of repeats 2..R,
the same rule for every runtime (JIT, interpreter or AOT): repeat 1 includes JIT warm-up, first-touch page faults and
lazy initialisation where a runtime has them, and is reported apart as `first_ms`. Single-threaded cells pinned to
CPU 0; T threads on CPUs 0, 2, .., 2T-2 (one logical CPU per physical P-core). CPU 0 is also where Windows runs much
of its interrupt and timer work, so a single-threaded cell shares its core with that housekeeping; every runtime
runs on the same CPU, so it is the same for all of them, and a quiet machine keeps it small. The checksum must
equal `expected.json` (quick sizes) and every other runtime's for the same kernel and size. A builtins cell
(`EXPECTED_REQUIRED` in `harness/session.py`) fails when `expected.json` has no entry for its kernel and size,
since `sync.py --expect` writes one for every way at every size; elsewhere a missing entry only skips that
comparison. Runtimes rotate order per round. Output: `results/<session>/kernels.jsonl` (one row per
cell, with the program's `extra` when it prints one), `summary.csv`, `meta.json` (versions, machine, sizes, git
commit). `run.py bench --group work` runs one group.

## 10. Lint and format (code may be shown on screen)

Zero warnings: Rust `cargo fmt --check` + `cargo clippy -- -D warnings`; Go `gofmt -l` + `go vet` + `staticcheck`;
C# `dotnet format --verify-no-changes` (+ build with warnings as errors); Java google-java-format (`--dry-run
--set-exit-if-changed`); JS Biome (`biome check`, defaults); Python `ruff check` + `ruff format --check`.
`run.py lint` runs them all.

## 11. Work kernels (group `work`): everyday jobs with each language's usual libraries

The question here is "how fast is the code a developer would actually write", so each kernel uses the most-used
library of the language (the standard library where it has one), called the way its README shows, with stock settings.
Where a language has a well-known alternative, a second kernel `<name>-alt` swaps exactly that library and nothing
else (`kernels.json` `"langs"` lists who has one). These rows compare libraries as much as languages: say so on screen.

All inputs are files in `kernels/data/`, written once by `python/gen_work.py` (standard library) and `python/photos.py`
(Pillow, in the py-work venv); `run.py` and `check.py` create them on demand (`harness/data.py`). Every program
reads the same files. The input is read into memory before timing (photos, corpus) or, for the 1 GB files, streamed
from the file inside the timed region (CSV, word count: reading is part of the job; the file sits in the OS page
cache after the first cell, and the median skips the first repeat).

### Inputs

| file | contents |
|---|---|
| `photos/photo-{0..7}.jpg` | 8 synthetic 24 MP photos, 6000 x 4000, JPEG quality 90, 4:2:0, about 7.6 MB each: per channel a fractal noise field (xorshift32 grids upscaled bicubic and blended coarse to fine), a sky gradient, 40 hard-edged shapes, a light blur and a tiled sensor grain (`photos.py`; seed `2463534242 + 7919 * index`). Generated, so there is no licence to track; `photos/SHA256SUMS` records the bytes (Pillow version dependent). Kernel size N = number of photos, photo `i % 8`. |
| `corpus-{N}.bin` | N bytes of mixed text: rounds of 400 access-log lines with timestamps, 100 JSON order lines (the `orders` objects of section 2, one per line) and 10 Python-like functions with generated identifiers, cut at N bytes (`gen_work.corpus`). |
| `orders-{N}.csv` | header `id,customer_id,status,total_cents,items,note`, then for i = 1..N: `i, 1 + (i * 48271) % 100000, status[i % 4], 500 + (i * 7919) % 50000, 1 + (i * 31) % 9, note`, note = `"Leave at the door, ring twice"` (quoted, comma) when i % 10 == 0, `"Box marked ""fragile"""` (escaped quotes) when i % 10 == 1, else `standard`. 24M rows = 1.04 GB. |
| `text-{N}.txt` | a 1 MiB block repeated N / 1 MiB times. Block: a vocabulary of 20,000 words (2..10 letters a-z from xorshift32), lines of 1..15 words joined by single spaces, word index `(a % 20000) * (b % 20000) / 20000` for two generator values a, b (a skew toward short indexes, like real text), padded to exactly 1 MiB with newlines. |

### Kernels and checksums

| kernel | timed work | checksum | `extra` |
|---|---|---|---|
| `image`, `image-alt` | for each of N photos in memory: decode, resize to 1080 px wide (height `(h * 1080 + w / 2) / w` = 720), encode JPEG quality 80 to memory | `"{N}:{width}x{height}"` of the last output | `out_bytes`: total JPEG bytes |
| `gzip` | gzip level 6 of the corpus (in memory) | `"{len}:{sha256 hex}"` of the output decompressed after timing | `compressed_bytes` |
| `gunzip` | decompress a gzip stream of the corpus made before timing | `"{len}:{sha256 hex}"` | |
| `zstd`, `unzstd` | the same with zstd level 3 | as gzip | `compressed_bytes` |
| `bcrypt` | N passwords `user-{i}-password`, cost 10, random salt each (the library's own) | `"{N}:{ok}:{ref}"`, ok = hashes that carry cost 10 and verify with the same library after timing, ref = 1 if the library verifies `$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy` for `correct horse battery staple` | |
| `argon2id` | the same passwords, argon2id v19, m = 19,456 KiB, t = 2, p = 1 (OWASP's first recommended setting), 16-byte salt, 32-byte hash, PHC string | `"{N}:{ok}:{ref}"`, ok = PHC strings starting `$argon2id$v=19$m=19456,t=2,p=1$` that verify, ref = the library verifies `$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk` | |
| `csv`, `csv-alt` | open `orders-N.csv`, parse every record with the CSV library (header by name), sum `total_cents` per `customer_id` in a hash map | `"{rows}:{customers}:{sum}:{top id}:{top sum}"`, top = largest sum, ties to the smaller id | |
| `wordcount` | read `text-N.txt` line by line, split on spaces, count each word in the standard hash map (string keys) | `"{words}:{distinct}:{top word}:{top count}"`, ties to the smaller word | |

Pixel output differs by library (decoder, resampler, encoder), so the image checksum checks only the count and the
output size; `out_bytes` shows the encoder difference. Password hashes are salted, so the checksum is the verify count
plus a reference vector that every library must accept (it proves the formats agree). Times are per call over N items;
per-photo or per-hash time is `median_ms / N`.

### Libraries (versions pinned in Cargo.lock, go.sum, Kernels.csproj, pom.xml, package-lock.json, requirements-work.txt)

| | image | image-alt | gzip | zstd | bcrypt | argon2id | csv | csv-alt | wordcount |
|---|---|---|---|---|---|---|---|---|---|
| Rust | `image` 0.25.10 (zune-jpeg decode), `resize_exact` Lanczos3, `jpeg-encoder` 0.7.1 (pure Rust) encodes 4:2:0 | `fast_image_resize` 6.1.0, Lanczos3 convolution (SIMD); decode/encode as `image` | `flate2` 1.1.11, default backend miniz_oxide | `zstd` 0.14.4 (libzstd 1.5.7), `bulk::compress` / `bulk::decompress` into a buffer of the frame's content size | `bcrypt` 0.19.3 | `argon2` 0.6.0 (RustCrypto) | `csv` 1.4.0 + serde struct | - | `BufRead::lines`, `split_ascii_whitespace`, `HashMap<String, u64>` (SipHash) |
| Go | `image/jpeg` + `x/image/draw` v0.48.0 CatmullRom (x/image has no Lanczos; CatmullRom is its best) | `disintegration/imaging` v1.6.2 Lanczos (pure Go) | `compress/gzip`; `gzip-alt`/`gunzip-alt`: `klauspost/compress/gzip` v1.20.4 | `klauspost/compress/zstd` v1.20.4 (pure Go; level 3 = SpeedDefault), content checksum off (`WithEncoderCRC(false)`; libzstd writes none by default) | `x/crypto/bcrypt` v0.59.0 | `x/crypto/argon2` IDKey (PHC string written by our code) | `encoding/csv` | - | `bufio.Scanner` + `ScanWords`, `map[string]int` |
| C# | SkiaSharp 4.156.0, `SKBitmap.Resize` CatmullRom | NetVips 3.2.0 (libvips 8.18.7) `ThumbnailBuffer`, Lanczos3 | `GZipStream` (zlib-ng in .NET 10), `ZLibCompressionOptions` level 6 | ZstdSharp.Port 0.8.8 (managed port of libzstd) | BCrypt.Net-Next 4.2.1 | Konscious.Security.Cryptography.Argon2 1.3.1 (PHC by our code) | CsvHelper 34.0.0 | Sylvan.Data.Csv 1.4.4 | `File.ReadLines`, `Split(' ')`, `Dictionary<string, long>` |
| Java | ImageIO + `Graphics2D.drawImage` bicubic, ImageIO JPEG writer 0.8, `ImageIO.setUseCache(false)` | Thumbnailator 0.4.21 (progressive bilinear, its default) | `GZIPOutputStream` (JDK zlib; its default level is 6) | zstd-jni 1.5.7-26 | spring-security-crypto 7.1.2 `BCryptPasswordEncoder` | spring-security-crypto `Argon2PasswordEncoder` (BouncyCastle 1.87) | Apache Commons CSV 1.14.1 | FastCSV 4.4.1 | `BufferedReader.readLine`, `split(" ")`, `HashMap.merge` |
| JS (Node, Bun, Deno) | sharp 0.35.5 (libvips 8.18.7) `resize(1080)`, Lanczos3, `optimiseCoding: false` | jimp 1.6.1 (pure JS), bicubic | `node:zlib` `gzipSync` level 6 | `node:zlib` `zstdCompressSync` level 3 | bcryptjs 3.0.3 (pure JS, the most downloaded); `bcrypt-alt`: bcrypt 6.0.0 (native) | argon2 0.45.2 (native, reference C) | csv-parse 7.0.3, streaming, `columns: true` | papaparse 5.7.1, streaming, `header: true` | `readline`, `split(" ")`, `Map` |
| Python (CPython 3.14) | Pillow 13.0.0 `resize` LANCZOS | pyvips 3.2.0 + pyvips-binary 8.18.7 `thumbnail_buffer`, Lanczos3 | `gzip.compress` level 6 (zlib) | `compression.zstd` (stdlib since 3.14) level 3 | bcrypt 5.0.0 (pyca) | argon2-cffi 25.1.0 (reference C) | `csv.reader` | polars 2.1.0 `scan_csv` + one `group_by` with `sum` and `pl.len()` (rows = the sum of the counts; Rust engine, lazy) | `for line in f`, `str.split`, `dict` |

### Settings a commenter could contest (all stock, written down)

- **Shrink-on-load.** libvips (`sharp`, `pyvips`, `NetVips`) decodes the JPEG at 1/2, 1/4 or 1/8 scale when the
  target is that much smaller, then finishes with Lanczos3, so it never builds the full 24 MP image.
  Pillow, `image`, Go, Skia, ImageIO, Thumbnailator and jimp decode every pixel. This is the libraries' default and
  the main reason libvips wins; say it on screen (Pillow's `draft()` would do the same, not used: not its default).
- **libvips' operation cache is off** (`sharp.cache(false)`, `pyvips.cache_set_max(0)`, `NetVips.Cache.Max = 0`):
  with it on, a warm repeat on the same buffer could return the cached result. The one change from stock, made so the
  repeats measure work.
- **JPEG encoders.** Every encoder writes quality 80 with 4:2:0 chroma subsampling except jimp, which has no option
  for it and writes 4:4:4 (twice the chroma samples to encode, a larger file). Rust's `image` 0.25 encoder also writes
  4:4:4 only, so Rust encodes with `jpeg-encoder` (4:2:0) instead. Huffman tables: sharp turns on libvips'
  `optimize_coding` by default while pyvips and NetVips leave it off, so sharp is told `optimiseCoding: false` and
  the three libvips rows do the same work; the other encoders keep their own defaults.
  Java's ImageIO writes through a disk cache by default; `ImageIO.setUseCache(false)` keeps it in memory like the
  others.
- **Resamplers differ** where a library has no Lanczos: Go `x/image` CatmullRom, Skia CatmullRom, Java2D bicubic
  (one pass, aliasing at 5.5x), Thumbnailator progressive bilinear, jimp bicubic. Quality is not compared, only time
  and output size.
- **Threads.** sharp/libvips, polars and Go's `imaging` size their thread pools from the machine; the runner pins
  single-threaded cells to one CPU, so they get one core like everyone else (polars is also told
  `POLARS_MAX_THREADS = --threads`).
- **gzip implementations differ**: miniz_oxide (Rust), Go's own deflate, zlib-ng (.NET), the JDK's zlib, Node's zlib,
  CPython's zlib. Same level 6, different compressed sizes (`compressed_bytes`). Rust has no `gzip-alt`: flate2
  picks its backend (miniz_oxide, zlib-rs, zlib-ng) at compile time, one per binary.
- **Java zstd** is JNI: the first call unpacks the native library to a temp folder, so the cold run and the first
  warm repeat are slower; the median skips it.
- **Password hashing** cost comes from the parameters, not the language. The parameters are identical in every row
  (bcrypt cost 10; argon2id m = 19,456 KiB, t = 2, p = 1), the cores are not: bcrypt is pure Rust (Rust, and Python's
  pyca bcrypt), Go, managed C# (BCrypt.Net), Java (Spring), pure JS (bcryptjs), OpenBSD's C++ in Node's native
  `bcrypt-alt`, Openwall crypt_blowfish in C and Botan in C++; argon2id is RustCrypto (Rust), `x/crypto` (Go),
  managed C# (Konscious), BouncyCastle (Java), the reference C code (Node `argon2`, Python argon2-cffi, C) and Botan
  (C++). Most libraries wrap the same C or Rust core
  (Python bcrypt is Rust, argon2-cffi and Node argon2 are the reference C code). bcryptjs, Go, Java and C# are written
  in their own language. C#'s Konscious Argon2 (the most-downloaded .NET package) is pure managed code; no native
  alternative is as widely used.
- **CSV parsers** do different amounts of work: polars converts only the two columns it needs (projection pushdown),
  csv-parse and papaparse build an object per row, Rust's serde struct ignores the other columns. That is how each is
  normally used.
- **Word count** keeps each language's plain idiom, including the per-word string allocation where the idiom has one
  (Go `scanner.Text()`, Java `split`, JS `split`, C# `Split`); Rust looks the word up as `&str` before allocating.
  Go's idiom is `bufio.Scanner` with `ScanWords`: it reads the file in blocks and splits on any whitespace (spaces
  and newlines alike), so it never builds a line; the others read a line, then split it on spaces. Same words, same
  counts, a little less work for Go.
- **zstd framing.** klauspost/compress writes a content checksum (xxhash64) into every frame by default and libzstd
  does not; Go turns it off, so no row writes or verifies one. Rust, C, Java and C# decompress in one call into a
  buffer of the size the frame records.

### Sizes

| kernel | full | quick | probe |
|---|---|---|---|
| image, image-alt | 8 photos | 1 | 2 |
| gzip, gunzip, gzip-alt, gunzip-alt, zstd, unzstd | 100 MiB | 1 MiB | 10 MiB |
| bcrypt, bcrypt-alt, argon2id | 32 passwords | 2 | 4 |
| csv, csv-alt | 24,000,000 rows (1.04 GB) | 20,000 | 2,400,000 |
| wordcount | 1 GiB | 8 MiB | 128 MiB |

Windows: every library above ships Windows x64 builds (wheels, npm prebuilds incl. `@img/sharp-win32-x64`, argon2 and
bcrypt `prebuilds/win32-x64`, zstd-jni's `win/amd64` DLL, SkiaSharp.NativeAssets.Win32, NetVips.Native.win-x64) or is
pure Rust/Go/C#/Java/JS; `zstd-sys` compiles libzstd with MSVC. pyvips itself is a pure-Python sdist that uses the
libvips in pyvips-binary. Dropped libraries and why: `csharp/DROPPED.md`, `go/DROPPED.md`.

## 12. Algorithm kernels (group `algo`)

Classic algorithms every programmer meets in a course or an interview: graphs, trees, dynamic programming, strings,
recursion and binary search. Each is hand-written the same way in all six languages with the plain data structures of
the language (`python/algo.py` is the reference; `rust/src/algo.rs`, `go/algo.go`, `csharp/Algo*.cs`,
`java/.../Algo*.java`, `js/algo.mjs`). Where the algorithm needs a standard container (queue, heap, ordered map,
linked map, binary search), it uses the one the language ships, and says so below; that choice is the part a
commenter can argue with. No SIMD, no `unsafe`, no node pools for the trees (the "Arena rows" below are the
exception, kept apart on purpose). Every runtime in `runtimes.json` runs the group (none
lists it in `skip_groups`; `py-numpy` and `py-work` run only their own kernels); `ordered-map` runs only where a
standard ordered map exists (`"langs"`).

Inputs come from the xorshift32 generator of section 2, a fresh generator per input, values used in the order
written. `h31` and the integer arrays are those of section 2. The checksum is computed inside the timed call (it is
a small part of the work), so Rust frees each tree there too.

| kernel | input (N = `--size`) | timed work | checksum |
|---|---|---|---|
| `bfs-grid` | N x N grid, row-major, cell is a wall when `next() % 100 < 30`; cells 0, 1, N, N*N-N-1, N*N-2, N*N-1 forced open (both corners) | BFS from cell 0, 4 neighbours (up, down, left, right), distance array of N*N (-1 = unseen), the language's FIFO queue | `"{reached}:{dist of the last cell}:{sum of distances}"` |
| `dijkstra` | N nodes, four edges per node u stored at 4u..4u+3: to `(u+1) % N` with `w = 1 + next() % 100`, then three times `v = next() % N`, `w = 1 + next() % 100` | Dijkstra from node 0 with the standard binary heap and lazy deletion (push on every improvement, skip stale pops) | `"{reached}:{sum of distances}:{max distance}"` |
| `kruskal` | 4N edges, each `u = next() % N`, `v = next() % N`, `w = next() % 1000000` (a fresh copy per repeat) | standard sort by weight, then union-find: find with full path compression (two passes, no recursion), union by rank | `"{forest edges}:{total weight}"` |
| `bst` | `hashmap(N)` of section 2 | insert every key into a hand-written unbalanced BST (node = key, count, left, right; a repeat bumps the count), then look up every `look` key; in-order walk with an explicit stack | `"{distinct keys}:{hits}:{h31 of the keys in order}"` |
| `ordered-map` | the same | the same with the standard ordered map: `m[k] += 1`, `contains`, keys in order | the same as `bst` |
| `trie` | N words (`len = 3 + next() % 8`, letters `'a' + next() % 26`), then N prefixes (`len = 1 + next() % 4`, same letters) | insert every word into a trie with 26 child slots per node, each node counts the words passing through; then sum the count at the end of each prefix (0 when absent) | `"{nodes created}:{sum of prefix counts}"` |
| `lru` | capacity C = max(1, N / 100); N keys `next() % (2C)` | for each key i: a hit moves it to most recent; a miss evicts the least recent when full, then inserts (value i) | `"{hits}:{misses}:{h31 of the cached keys, least recent first}"` |
| `edit-distance` | two strings of N letters `"acgt"[next() % 4]`, a then b | Levenshtein, two rows of N+1, `cur[j] = min(prev[j] + 1, cur[j-1] + 1, prev[j-1] + (a[i] != b[j]))` | the distance |
| `knapsack` | 1000 items: `weight = 1 + next() % 1000`, then `value = 1 + next() % 1000`; capacity N | 0/1 knapsack in one row, capacity from N down to the weight | `"{best value}:{h31 of the row}"` |
| `lis` | `ints(N)` | patience: `tails`, lower bound of x with the standard binary search (JS has none: hand-written), replace or append | `"{length}:{h31 of tails}"` |
| `kmp` | text = N letters as `edit-distance` (one string); pattern = text[N/2 .. N/2+8) | hand-written KMP (failure table, then one pass), every match, overlaps included | `"{matches}:{first index}:{last index}"` |
| `str-find` | the same | the standard substring search in a loop, restarting at the last match + 1 | the same as `kmp` |
| `fib` | N | naive recursion `fib(n) = n < 2 ? n : fib(n-1) + fib(n-2)` (one function call per node) | fib(N) |
| `nqueens` | N | backtracking row by row with three boolean arrays (columns, both diagonals), recursive | number of solutions |
| `binary-search` | sorted array: `a[i] = a[i-1] + 1 + next() % 4` (a[-1] = 0), then N queries `next() % (a[N-1] + 1)` | hand-written lower bound (`mid = (lo + hi) / 2`) for each query | `"{hits}:{sum of lower-bound indexes}"` |

### Standard containers and the choice a commenter could contest

| kernel | Rust | Go | C# | Java | JS | Python | contestable |
|---|---|---|---|---|---|---|---|
| `bfs-grid` | `VecDeque<usize>` | slice + head index (the Go idiom; no std deque) | `Queue<int>` | `int[]` of N*N + head index | Array + head index (`shift()` is O(n)) | `collections.deque` | Java has no `int` queue (`ArrayDeque<Integer>` would box every cell), so it uses a plain `int[]` with head and tail indexes like Go and JS; each cell is queued at most once |
| `dijkstra` | `BinaryHeap<Reverse<Visit>>`, `Visit` ordered by distance only (as Go and Goose compare; a tuple would compare the node too) | `container/heap` | `PriorityQueue<int, long>` | `PriorityQueue<Entry>` (record) | hand-written binary heap of `[dist, node]` (JS has none) | `heapq` with tuples | Go's `container/heap` goes through an interface (`Push(any)` boxes each item); a typed heap would skip that. Java allocates a record per push |
| `kruskal` | `sort_unstable_by_key` | `slices.SortFunc` | `Array.Sort` with a comparison | `Arrays.sort` + `Comparator.comparingInt` (objects, TimSort) | `sort((a, b) => a.w - b.w)` on objects | `list.sort(key=itemgetter(0))` on tuples | edges are structs in Rust/Go/C#, objects in Java/JS/Python; sorting dominates the time; Rust, Go, C# and Goose sort unstably, Java, JS and Python with stable TimSort (equal weights may come out in another order, the forest weight is the same) |
| `bst` | `Option<Box<Node>>` | `*node` | class | class | class | class with `__slots__` | unbalanced on purpose (random keys give depth ~50 at 1M); Rust frees the tree inside the timed region, GC languages may not. Goose has no heap: not run here, see `bst-arena` |
| `ordered-map` | `BTreeMap` (B-tree) | none: not run | `SortedDictionary` (red-black) | `TreeMap<Long, Long>` (red-black, boxed) | none: not run | none in the stdlib: not run | different trees (B-tree vs red-black); Java boxes keys and values; C# updates with two lookups (`m[k] = GetValueOrDefault(k) + 1`) as in `hashmap` |
| `trie` | `[Option<Box<_>>; 26]` | `[26]*trieNode` | `TrieNode?[26]` | `TrieNode[26]` | `Array(26)` | list of 26 | a dict/Map of children is the more common idiom in Python and JS; the 26-slot array keeps the same structure everywhere. Goose has no heap: not run here, see `trie-arena` |
| `lru` | no linked map in std: `HashMap<u32, usize>` + doubly linked list of indices in a `Vec` | `container/list` + `map` | `LinkedList<int>` + `Dictionary` | `LinkedHashMap` in access order, `removeEldestEntry` | `Map` from key to node + hand-written doubly linked list | `OrderedDict` (`move_to_end`, `popitem(last=False)`) | each language's usual LRU, so the structures differ; Rust's `HashMap` is SipHash; Java boxes `Integer` keys |
| `edit-distance` | `Vec<usize>` | `[]int` | `int[]` | `int[]` | `Int32Array` | list | JS uses a typed array (numbers in fixed arrays), Python a list |
| `knapsack` | `Vec<u64>` | `[]int64` | `long[]` | `long[]` | `Float64Array` | list | as above |
| `lis` | `partition_point` | `slices.BinarySearch` | `Array.BinarySearch(tails, 0, len, x)` | `Arrays.binarySearch(tails, 0, len, x)` | hand-written lower bound | `bisect_left` (C) | Python's `bisect` is C code, so this row is closer than the others. Rust's `partition_point` is branchless (a fixed number of steps with conditional moves, no mispredicted branch per step), the others branch. `tails` grows by doubling in every language (Rust `Vec`, Go `append`, C# and Java from 16), never an N-sized array up front |
| `kmp` | bytes | bytes of a string | `string` chars | `String.charAt` | `charCodeAt` | iterates the `str` | |
| `str-find` | `str::find` (Two-Way) | `strings.Index` | `IndexOf(p, start, Ordinal)` | `String.indexOf` | `indexOf` | `str.find` | library code (assembly or vectorised in Go, .NET and HotSpot, C++ in the JS engines, C in CPython): compares each runtime's string search, not the language; show it beside `kmp`. Goose's `std::find` is the naive search (compare the pattern at every position, O(N*M) worst case), so its row measures that loop |
| `fib` | `u64` | `int` | `long` | `long` | number | int | measures function-call cost; Python's recursion limit is not touched (depth N = 40) |
| `nqueens` | `Vec<bool>` in a struct | `[]bool` | `bool[]` + local function | `boolean[]` | Array of booleans + closure | list + nested function | arrays, not the bitmask version (faster, less readable); recursion depth N |
| `binary-search` | `&[i64]` | `[]int64` | `long[]` | `long[]` | Array | list | hand-written in all six (the point is the loop) |

### Follow-up rows (opt-in: run only when named with `--kernel` / `--runtime`)

Kernels and runtimes marked `"opt_in": true` in kernels.json / runtimes.json never join a session by default; a
session names them. `run.py bench --warm-repeats R` fixes the warm process's repeats (default: the 10 s budget).

- `kmp-en`, `str-find-en`: `kmp` and `str-find` on English-like text instead of DNA, same N, same pattern rule
  (text[N/2 .. N/2+8)), same checksum format. Text: a fresh generator; until the text has N bytes: `a = next() % 64`,
  `b = next() % 64`, append `WORDS[min(a, b)]`, then `r = next() % 16`: `". "` if r == 0, `", "` if r == 1, else
  `" "`; cut to N bytes. `WORDS` are the 64 most common English words in the order of the Oxford English Corpus list
  (Wikipedia, "Most common words in English"): the be to of and a in that have i it for not on with he as you do at
  this but his by from they we say her she or an will my one all would there their what so up out if about who get
  which go me when make can like time no just him know take people into year your. The min of two draws makes early
  (frequent) words come up more often, roughly like real text. All 21 core runtimes have both.
- `kruskal-split` (Python only): `kruskal`, with the sort and the union-find timed apart (`extra`: `sort_ms`,
  `union_find_ms`, from the last repeat). `kruskal-packed`: each edge packed into one int `w << 42 | u << 21 | v`
  (N < 2^21) before timing, a plain `list.sort()`, then the same union-find unpacking the key; same checksum (ties
  may come out in another order; the forest weight is the same). `kruskal-wsort`: only the 4N weights, a plain int
  sort; checksum `"{4N}:{h31 of the sorted weights}"`.
- Runtime variants (the same binary with one environment switch, or one build flag):
  `c-clang-edleft` (`KERNELS_ED_MIN=left-last`: `min3(up, diag, left)`), `cpp-msvc-edmin2` / `cpp-clang-edmin2`
  (`KERNELS_ED_MIN=min2`: `std::min(std::min(up, left), diag)` in place of `std::min({...})`), `cpp-msvc-novec`
  (`./build.sh msvc-novec`: everything built with `/D_USE_STD_VECTOR_ALGORITHMS=0`), `cpp-*-bmh` /
  `cpp-*-strstr` (`KERNELS_STRFIND`: `std::search` with a `std::boyer_moore_horspool_searcher` built inside the
  timed call, or `std::strstr` on the same NUL-terminated string), `go-typedheap` (`KERNELS_DIJKSTRA_HEAP=typed`: a
  generic binary heap `binHeap[item]`, no `container/heap`), `go-heap32` / `go-typedheap32`
  (`KERNELS_DIJKSTRA_HEAP=container32` / `typed32`: `container/heap` or `binHeap` with C#'s types, `int32` edge
  targets and node ids, `int64` weights and distances), `go-swapheap` (`KERNELS_DIJKSTRA_HEAP=typedswap`: heap.go's own
  swap-based `up`/`down` with a type parameter, no `heap.Interface`, no `any`), `java-primheap` (`KERNELS_DIJKSTRA_HEAP=primitive`: a
  binary heap in two parallel `long[]`/`int[]` arrays), `node-hllclz` / `deno-hllclz` (`KERNELS_HLL=clz`:
  `Math.clz32(h << 14)` without `>>> 0`). Checksums are those of the base kernel.
- `extra` reported by base rows: C++ `str-find` / `str-find-en` (std::string_view::find) count, after timing, the
  positions holding the pattern's first letter (`first_letter_hits`) and those where no match starts
  (`false_starts`); Go `dijkstra` reports `mallocs_per_run`, `alloc_mb_per_run`, `gc_cycles_per_run`
  (`runtime.MemStats` around all repeats, divided by them); Java `dijkstra` reports `allocated_bytes` of the last
  repeat (`com.sun.management.ThreadMXBean.getCurrentThreadAllocatedBytes`, -1 where the VM does not report it).

### Arena rows (`bst-arena`, `trie-arena`, `binary-trees-arena`)

The same algorithm, input, sizes and checksum as `bst`, `trie` and `binary-trees`, with every node in one growable
array and 32-bit index links instead of one allocation per node: what an arena buys each language, and the only form
Goose can run (it has no heap; its plain rows are not run). Run by Rust (and the Rust -> Wasm runtimes), Go, C#,
Java, JS and Goose (`"langs"` in kernels.json); Python has no row. The array starts with room for 16 nodes in
Rust, Go, C, C#, Java and JS (`Vec::with_capacity(16)`, `make(.., 0, 16)`) and grows when full: Rust's `Vec::push`, C,
C#, Java and JS double it, Go's `append` doubles up to 256 elements and then grows by about 1.25x; a new array per
tree in `binary-trees-arena`,
children pushed before their parent, as the pointer versions allocate them.

| | Rust | Go | C# | Java | JS | Goose |
|---|---|---|---|---|---|---|
| node storage | `Vec<Node>` (struct) | `[]node` (struct) | struct array (`[InlineArray(26)]` children in the trie) | `int[]`/`long[]`: four parallel arrays (bst), 27 ints per node (trie), 2 per node (binary-trees) | typed arrays laid out as in Java (`Float64Array` keys) | one pool vector of structs |
| link | `u32` index | `int32` index | `int` index | `int` index | `Int32Array` entry | 4-byte self-relative link |

"No child" is -1 (Rust `u32::MAX`) in the BST and binary trees; in the trie it is 0, since node 0 is the root and
never a child. The checksum walk (the BST's in-order keys) stays inside the timed call, as in the pointer rows.

### Sizes

| kernel | full | quick | probe | why the full size |
|---|---|---|---|---|
| bfs-grid | 5,000 (25M cells) | 200 | 1,600 | CPython about 10 s |
| dijkstra, kruskal | 2,000,000 nodes | 20,000 | 200,000 | Python holds 8M edges (about 1 GB of tuples for kruskal) |
| bst, ordered-map, bst-arena | 1,000,000 | 20,000 | 100,000 | pointer chasing past the caches |
| trie, trie-arena | 500,000 words | 20,000 | 50,000 | about 1.5M nodes (300-500 MB) |
| lru | 10,000,000 ops | 100,000 | 1,000,000 | |
| edit-distance | 10,000 | 500 | 3,200 | 100M cells; CPython about 15 s |
| knapsack | 200,000 | 2,000 | 20,000 | 200M cells; CPython about 15 s |
| lis | 10,000,000 | 100,000 | 1,000,000 | |
| kmp, str-find | 50,000,000 letters | 100,000 | 5,000,000 | building the text costs CPython about 10 s per process (not timed) |
| fib | 40 | 20 | 40 | 331M calls; fib(42) took CPython 3.12 over a minute on the Mac |
| nqueens | 13 | 8 | 13 | 14 would take CPython about 40 s |
| binary-search | 2,000,000 | 20,000 | 200,000 | CPython 3.12 about 15 s; the 16 MB array is bigger than the i7's L2, about half its L3 |

The fast runtimes finish most rows in 0.1 to 1 s; a bigger N would make CPython (four runtimes, 20-40x to 100x
slower here) dominate the session. `fib` and `nqueens` grow exponentially, so their probe size is the full size
(cost model `const`).

## 13. Unusual algorithm kernels (group `puzzle`)

Little-known algorithms that are interesting on their own: each row of the video teaches one trick and races it in
six languages. Single-threaded, standard library only, plain loops and arrays; the steps below are the same in every
language (the reference is `python/puzzle.py`, with `sudoku` in `python/sudoku.py`), so the counts in the checksums (search tries, false positives,
rounds) agree exactly. Each program keeps the group in its own files (`puzzle.rs` + `puzzle_text.rs`, `puzzle.go` + `puzzle_text.go`,
`Puzzle.cs` + `PuzzleText.cs`, `Puzzle.java` + `PuzzleText.java`, `puzzle.mjs` + `puzzle-text.mjs`, `puzzle.py` +
`sudoku.py`; the second file holds the string kernels palindrome, bwt, aho-corasick and huffman, in C# also
pi-digits).

**fmix32(h)**, MurmurHash3's 32-bit finalizer, is the one hash of the probabilistic kernels (so every language sets
the same bits): `h ^= h >> 16; h *= 0x85ebca6b; h ^= h >> 13; h *= 0xc2b2ae35; h ^= h >> 16` (32-bit unsigned, products
mod 2^32; JS `Math.imul`). **shuffle(rng, a)** is Fisher-Yates: `for i = len-1 down to 1: j = next() % (i+1); swap
a[i], a[j]`. "Mod" below is the non-negative remainder; every intermediate fits in 53 bits (JS numbers are exact)
unless 32-bit wrap-around is stated.

| kernel | trick, in one sentence (for the video) | the stock choice a commenter could contest |
|---|---|---|
| `sudoku` | Sudoku is an exact-cover problem: 324 constraints, 729 candidate placements, and Knuth's dancing links remove and restore a placement by relinking four pointers, so the backtracking search undoes its work for free. | Flat int arrays for the links (the usual way outside C), not node objects; the search counts every solution instead of stopping at the first. |
| `bloom` | Hash each key to 7 bits of one bit array: if any of its 7 bits is 0 the key was never added, and if all are 1 it probably was, at about 1% false positives for 10 bits per key. | The 7 positions come from two hashes (`h1 + i*h2`, Kirsch-Mitzenmacher), not 7 independent hashes; fmix32 is a 32-bit hash. |
| `hyperloglog` | A random hash that starts with k zero bits turns up about once per 2^k distinct items, so the longest run of leading zeros, kept in 16,384 one-byte registers, estimates millions of distinct items from 16 KB within about 1%. | 32-bit hash (collisions add a small bias above ~10M items), no bias-correction table (HyperLogLog++). |
| `skiplist` | A sorted linked list where each node flips coins for its height: the tall nodes form express lanes, so search skips ahead like a binary search with no rebalancing ever. | p = 1/2, max height 24; Rust keeps nodes in a Vec with index links (safe Rust has no shared mutable pointers). |
| `cycle` | A sequence that repeats must loop, and a tortoise moving one step while a hare moves two meet inside the loop; a second walk from the start then finds where the loop begins, with O(1) memory. | Floyd's algorithm, not Brent's (fewer function calls). |
| `palindrome` | Inside a long palindrome the right half mirrors the left, so Manacher's algorithm copies each radius from its mirror and only extends past the right edge, finding every palindrome in linear time. | The `#`-separated transformed string (one array for odd and even palindromes) instead of two radius arrays. |
| `bwt` | Sort all suffixes by their first 1, 2, 4, 8, ... characters, each round reusing the ranks of the last; the character before each sorted suffix is the Burrows-Wheeler transform, the index DNA aligners such as BWA and Bowtie search. | O(n log^2 n) prefix doubling with the standard comparison sort (Java sorts boxed `Integer[]`, it has no primitive sort with a comparator), not linear SA-IS. |
| `aho-corasick` | Put all 10,000 patterns in one trie and give each node a failure link to the longest suffix that is also in the trie: the text is read once, one table step per character, however many patterns there are. | A dense 4-column transition table (DNA alphabet) with the failure links folded in (a DFA); match counts summed along output links at build time. |
| `pi-digits` | The Chudnovsky series adds about 14 correct digits of pi per term, and binary splitting sums all its terms as one exact fraction P/Q built from a tree of big-integer products, so the race is really each language's big-number multiply and divide (the square root is built from them). | Each language's standard big integer (Python `int`, Go `math/big`, Java `BigInteger`, C# `System.Numerics.BigInteger`, JS `BigInt`; Rust has none, so `num-bigint` 0.5.1, the most used crate, pinned in Cargo.toml), none backed by GMP. The square root is the one hand-written function: the same precision-doubling integer square root (CPython's `math.isqrt` algorithm, about log2(bits) steps that each double the correct bits) in all six, not each library's own (Go `Sqrt`, Java `BigInteger.sqrt`, `num-bigint`'s `sqrt`, Python's C `math.isqrt`) or a Newton loop; the decimal conversion is timed. |
| `life` | Four rules on a grid of cells (born with 3 neighbours, survives with 2 or 3) are enough to make gliders and oscillators: a computer with no program but the rules. | A byte per cell and a modulo per neighbour lookup for the torus, no bit packing or HashLife. |
| `huffman` | Merge the two rarest symbols into one node until one tree is left: frequent letters end up near the root with short codes, which is provably the shortest prefix code. | The standard priority queue (JS has none: a hand-written binary heap), bit-at-a-time tree walk to decode (no lookup tables). |
| `window-max` | Keep the indexes of a window in a deque whose values only decrease: a new value removes every smaller one from the back, so the front is always the maximum and each element enters and leaves once, for any window size. | A plain array with head and tail indexes as the deque in every language, not Rust's `VecDeque`, Java's `ArrayDeque` or Python's `deque`. |
| `rain-water` | Water can only escape over the lowest wall, so pour from the border inward with a min-heap: every cell popped from the heap fixes the water level of its neighbours. | The heap holds one packed integer `height * n + index` (no tuples or objects); Java's `PriorityQueue<Long>` boxes it, and Go's `container/heap` takes and returns `any` (`Push(any)`, `Pop() any`), so each int64 is boxed into an interface value too. |
| `collatz` | Every Collatz walk eventually drops below its start, so caching each chain length lets the walk stop at the first smaller number instead of going all the way down to 1. | A cache of chain lengths only below N, not a hash map for the large values; the cache is 32-bit in every language. |

### Inputs and steps

Every input starts from a fresh XorShift32 with the section 2 seed unless a seed is given. Checksums are computed
after the timer in every language (section 1): the timed call returns its counters and result arrays, and the
hashes over whole results (`bwt`'s and `huffman`'s fnv1a, `life`'s alive count and fnv1a, `skiplist`'s level-0
walk, `pi-digits`' digit sum) run once the clock has stopped. Unlike the algo group, no hash over a large result is
timed here.

- `sudoku` N puzzles, 26 clues each, from one generator. Per puzzle: `digits = shuffle([1..9])`; then for rows and
  then columns: `bands = shuffle([0,1,2])`, and for each band in that order `inner = shuffle([0,1,2])`, lines +=
  `band*3 + inner[i]`; that gives `rows[9]` and `cols[9]`; `cells = shuffle([0..80])`; for the first 26 cells:
  `r = rows[cell/9], c = cols[cell%9]`, `puzzle[cell] = digits[(r*3 + r/3 + c) % 9]` (a permuted valid grid).
  Timed, per puzzle: build the dancing-links matrix in flat int arrays: node 0 root, 1..324 column headers in a
  circular list, then for r, c, d = 0..8 (nested in that order) the four nodes of row `(r*9+c)*9+d`, columns
  `1+r*9+c`, `82+r*9+d`, `163+c*9+d`, `244+box*9+d` (box = (r/3)*3 + c/3), each appended at the bottom of its column
  and linked left-right in that order. For each clue (cell order): cover its row's first node's column, then the
  columns of the row's other nodes going right. Search (recursive): no column left -> a solution (keep the first);
  else pick the column with the fewest nodes (first in header order on ties), cover it, and for each row top to
  bottom: tries += 1, cover the other columns going right, recurse, uncover going left; then uncover the column.
  `cover`/`uncover` are Knuth's (rows top-down / bottom-up, nodes right / left). Checksum
  `"{N}:{solutions}:{tries}:{h}"`, h = h31 chained over puzzles: `h = h31([h, 81 digits of the puzzle's first solution])`.
- `bloom` N: `values = ints(2N)`, keys = the first N, lookups = the next N (xorshift never repeats, so every lookup
  hit is a false positive). m = 10N bits in a byte array (`bits[b>>3] |= 1 << (b&7)`), k = 7, `h1 = fmix32(key)`,
  `h2 = fmix32(key ^ 0x9e3779b9) | 1`, bit i = `((h1 + i*h2) mod 2^32) mod m`. Insert all keys, then test each lookup
  (stop at the first 0 bit). Checksum `"{N}:{bits set}:{false positives}"` (bits set by popcount at the end, timed).
- `hyperloglog` N: timed, the stream is generated inline: N items `next() % max(N/4, 1)`, h = fmix32(item),
  register `h >> 18` (p = 14, 16,384 byte registers), rank = leading zeros of `(h << 14) mod 2^32` + 1, or 19 when
  that is 0; keep the max. Estimate: `z = sum of 2^-reg` (register order, doubles), `E = 0.7213 / (1 + 1.079/m) * m
  * m / z` (left to right), if `E <= 2.5m` and V zero registers > 0: `E = m * ln(m / V)`. Checksum
  `"{N}:{floor(E + 0.5)}:{h31(registers)}"`.
- `skiplist` N: keys = ints(N), lookups as `hashmap` (section 2). Heights from a second generator seeded `88675123`:
  `height = 1; while height < 24 and next() & 1: height++`, drawn only when a key is inserted. Insert: from the
  current top level down, move right while the next key is smaller, remember the node per level; equal key -> skip;
  new levels above the current top start at the head. Then search every lookup the same way. Checksum
  `"{N}:{hits}:{sum of heights}:{h31(keys along level 0)}"` (the walk is after timing).
- `cycle` N starts `next() % M`, `f(x) = (x*x + 12345) mod M`, M = 67,108,859. Per start x0 (Floyd): `t = f(x0); h =
  f(f(x0)); while t != h: t = f(t); h = f(f(h))`; `mu = 0; t = x0; while t != h: t = f(t); h = f(h); mu++`; `lam = 1;
  h = f(t); while t != h: h = f(h); lam++`. Checksum `"{N}:{sum mu}:{sum lam}:{max mu+lam}"`.
- `palindrome` N: characters from the bits of generator values, low bit first, 32 per value: 1 -> `b`, 0 -> `a`.
  Timed: `t = "^#" + c0 + "#" + c1 + ... + "#$"` (2N+3 bytes), `p` = int array of that length; for i = 1 .. len-2:
  `if i < right: p[i] = min(right - i, p[2*center - i])`; `while t[i+1+p[i]] == t[i-1-p[i]]: p[i]++`; `if i + p[i] >
  right: center = i; right = i + p[i]`. Then longest = max p (first i), count = sum of `(p[i]+1)/2`. Checksum
  `"{N}:{longest}:{(i - 1 - longest)/2}:{count}"` (the start of the first longest palindrome).
- `bwt` N: DNA `ACGT[v & 3]` from 2-bit chunks of generator values, low bits first, 16 per value; text = DNA + `$`
  (n = N+1). Timed: `rank[i]` = byte value, `sa = 0..n-1`, k = 1; loop: rounds++, `second[i] = rank[i+k]` or -1 past
  the end; sort sa by `(rank[i], second[i])` with the standard sort and a comparator; new rank: `tmp[sa[0]] = 0`,
  `tmp[sa[j]] = tmp[sa[j-1]] + (pair differs)`; stop when `rank[sa[n-1]] == n-1`, else k *= 2. Then `bwt[j] =
  text[sa[j]-1]` (`text[n-1]` when sa[j] = 0, that j is `primary`). Checksum `"{n}:{rounds}:{primary}:{fnv1a(bwt)}"`.
- `aho-corasick` N bytes: 10,000 patterns, each `len = 6 + next() % 7` then `len` times `ACGT[next() % 4]`; then a
  1 MiB block made as `bwt`'s DNA from the same generator, repeated N / 1 MiB times. Timed: trie in a growing int
  table of 4 columns per state (-1 = no edge, state 0 the root), out[state] += 1 per pattern ending there; BFS from
  the root (the root's missing edges -> 0; its children's fail = 0, queued in column order); per dequeued state s:
  `out[s] += out[fail[s]]`, then for each column c: missing edge -> `goto[fail[s]][c]`, else `fail[t] =
  goto[fail[s]][c]`, queue t. Scan: `s = goto[s][code[byte]]`; `matches += out[s]`, `hits += 1` when out[s] > 0.
  Checksum `"{patterns}:{states}:{matches}:{hits}"`.
- `pi-digits` N digits (no input). Timed: `terms = N / 14 + 2`; binary splitting `bs(a, b)` returns P, Q, T:
  for `b - a == 1`: `P = Q = 1` when a = 0, else `P = (6a-5)(2a-1)(6a-1)`, `Q = a^3 * 640320^3 / 24`
  (= 10939058860032000); `T = P * (13591409 + 545140134 a)`, negated when a is odd; otherwise `m = (a + b) / 2`,
  `P = P1 P2`, `Q = Q1 Q2`, `T = T1 Q2 + P1 T2` (recursive, depth about 16). With `(P, Q, T) = bs(0, terms)`,
  `one = 10^(N+10)` (10 guard digits), `s = isqrt(10005 * one * one)` (floor square root, exact; written out in every language as
  `c = (bits(n) - 1) / 2; a = 1; d = 0; for s = bits(c) - 1 down to 0: e = d; d = c >> s; a = (a << (d - e - 1)) +
  (n >> (2c - e - d + 1)) / a`, then `a - 1` if `a*a > n`), `pi = 426880 * s * Q / T` (floor division, T > 0), then the decimal string of
  `pi / 10^10` (`"3"` and N decimals). Checksum `"{N}:{last 50 decimals}:{sum of all N+1 digits}"`. Reference: the
  1,000,000-digit run ends `...5779458151` (the published millionth digits), its first 500,001 digits equal the
  500,000-digit run, and the 10,000- and 20,000-digit runs match an independent Machin-formula computation.
- `life` N generations on a 512 x 512 torus; cell alive when `next() % 4 == 0` (row-major). Timed: copy into a
  byte array, then per generation for y, x: up/down rows `(y + w - 1) % w`, `(y + 1) % w`, left/right `(x + w - 1)
  % w`, `(x + 1) % w`, sum the 8 neighbours, `next = n == 3 or (n == 2 and alive)`, swap the two grids. Checksum
  `"{N}:{alive}:{fnv1a(grid)}"`.
- `huffman` N bytes: a 1 MiB block of `97 + (next() % 26) * (next() % 26) / 26` (the two calls left to right),
  repeated N / 1 MiB times. Timed: byte counts; the priority queue holds `freq * 1024 + id` (leaves id = byte,
  internal nodes 256, 257, ...): pop a, pop b, node(left = a's id, right = b's id), push the sum; codes by walking
  the tree (left 0, right 1); encode: `acc = acc << len | code`, write whole bytes from the top, keep the rest, pad
  the last byte with zeros; decode bit by bit from the root (MSB first) until N bytes. Checksum
  `"{N}:{total bits}:{fnv1a(encoded)}:{1 if decoded == input else 0}"`. Codes are at most 12 bits on this text.
- `window-max` N: values = ints(N); for each window w in 10, 100, 1,000, 10,000, 100,000: `head = tail = 0`; for i:
  `while tail > head and v[dq[tail-1]] <= v[i]: tail--`; `dq[tail++] = i`; `if dq[head] <= i - w: head++`; for
  `i >= w-1`: `h = h31 step with v[dq[head]]` (dq is one int array of N, reused). Checksum `"{N}:{h_10}:...:{h_100000}"`.
- `rain-water` side S: heights = the first S*S values `% 1000`, row-major, n = S*S. Timed: push every border cell
  (index order) as `height * n + index`, mark seen; pop the smallest, `level = key / n`, for neighbours up, down,
  left, right inside the grid and unseen: mark, `h = height`; if `h < level`: water += level - h, wet += 1, h =
  level; push `h * n + neighbour`. Checksum `"{S}:{water}:{wet}"`.
- `collatz` N: `cache[1] = 1` (a 32-bit int array of N); for start = 2 .. N-1: `x = start; steps = 0; while x >= start: x
  = x/2 if even else 3x + 1; steps++`; `len = steps + cache[x]`, cache it, total += len, keep the first longest.
  Checksum `"{N}:{best start}:{best length}:{total}"` (64-bit x: the walks below 20M peak under 2^53).

### Sizes

| kernel | full | quick | probe |
|---|---|---|---|
| sudoku | 120 puzzles | 4 | 16 |
| bloom | 2,000,000 keys | 20,000 | 250,000 |
| hyperloglog | 12,000,000 items | 100,000 | 1,600,000 |
| skiplist | 1,000,000 | 20,000 | 40,000 |
| cycle | 4,000 starts | 20 | 100 |
| palindrome | 12,000,000 | 100,000 | 2,000,000 |
| bwt | 1,000,000 | 20,000 | 100,000 |
| aho-corasick | 48 MiB | 1 MiB | 8 MiB |
| pi-digits | 500,000 digits | 10,000 | 100,000 |
| life | 100 generations | 2 | 10 |
| huffman | 16 MiB | 1 MiB | 2 MiB |
| window-max | 8,000,000 | 100,000 | 800,000 |
| rain-water | 1,500 x 1,500 | 100 | 474 |
| collatz | 20,000,000 | 100,000 | 2,000,000 |

Sizes are set so CPython takes about 5-20 s per run (the session stays near 4 h for 3 rounds); the compiled
runtimes then take 0.05-1 s, JS and Java's boxed heaps up to a few seconds (the loops here are
the kind where an interpreter is 50-200x slower, so the usual 0.5-2 s for the fastest runtime would put CPython at
minutes per run).

## 14. Python compilers, JITs and worker pools (`py-nuitka`, `py-mypyc`, `py-cython`, `py-numba`, `py-cpython-jit`, `py-graalpy`, `py-subinterpreters`, `py-processes`)

Ways to make the same Python faster without rewriting it. Every one runs the reference sources in `python/`
unchanged; a small entry file per runtime puts the compiled modules (or the pool) in place, then calls `main.py`,
and adds what was used to the `runtime` field. A missing build fails the cell instead of passing as plain CPython.
Everything is CPython 3.14 (`$PY314`, the python.org build on the i7) except GraalPy. The compilers and Numba live
in one venv, `kernels/.venv-pyc`, pinned by `python/requirements-pyc.txt` (from `requirements-pyc.in`:
nuitka 4.3.5, mypy 2.4.1, cython 3.3.1, numba 0.68.1 + llvmlite 0.50.0 + numpy 2.5.4, setuptools 84.2.1).
`python/build_compiled.py mypyc|cython|nuitka` builds into `python/bin/<tool>/` and skips a build that is newer
than its sources (`--force` rebuilds). All work-group kernels are skipped (their libraries live in `.venv-work`).

| runtime | what | entry | kernels |
|---|---|---|---|
| `py-nuitka` | Nuitka 4.3.5 `--mode=standalone`: `compiled.py`, `main.py` and every module compiled to C and linked into one program with the Python runtime beside it (`bin/nuitka/compiled.dist/kernels.exe`). Stock options; Windows adds `--msvc=latest` (Nuitka's MinGW does not build 3.13+). | `compiled.py` | all |
| `py-mypyc` | mypyc (mypy 2.4.1): each kernel module that type-checks under mypy's stock settings becomes a C extension; `main.py` stays interpreted (it only times and prints). | `compiled.py mypyc` | sort, bg, lib, loop, mt |
| `py-cython` | Cython 3.3.1 `cythonize -i -3` on the unchanged `.py` files (pure-Python mode: no `.pyx`, no `cdef`, no `cython.*` types; Cython still reads the PEP 484 annotations). | `compiled.py cython` | all but `palindrome`, `life` |
| `py-numba` | Numba 0.68.1: `numba.njit` wrapped around the kernel's own functions, the same as writing `@njit` above them, and around the functions of that module they call. Nothing else changes. | `jitted.py` | the 20 below |
| `py-cpython-jit` | CPython 3.14 with `PYTHON_JIT=1`: the experimental copy-and-patch JIT the python.org Windows and macOS builds ship switched off. `jit.py` fails unless `sys._jit.is_enabled()`. | `jit.py` | all, threads 1-8 |
| `py-graalpy` | GraalPy 25.5.5 community (Python 3.13, Truffle JIT on the GraalVM native image), `graalpy main.py`, no flags. GIL like CPython, so `mt: false`. | `main.py` | all |
| `py-subinterpreters` | the three multi-threaded kernels on `concurrent.futures.InterpreterPoolExecutor` (3.14): each worker thread runs its own interpreter with its own GIL. | `pools.py interpreters` | mt |
| `py-processes` | the same on `ProcessPoolExecutor` (spawn on Windows and macOS). | `pools.py processes` | mt |

**Results that are not bugs.** Each is what the tool does to the code as written; say it on screen rather than hide it:

- **mypyc refuses code that does not type-check.** `algo.py` has 4 mypy errors (a variable that later holds
  `Node | None`, an unannotated empty list) and `puzzle.py` passes a `bytearray` where `fnv1a` says `bytes`
  (mypy 2's strict bytes). The build leaves those modules out (`bin/mypyc/modules.txt`), so `py-mypyc` skips the algo
  and puzzle groups. `python/NOTES-PYC.md` has an annotation-only fix: with it `algo.py` compiles and passes, while
  mypyc 2.4.1 then crashes on `puzzle.py` line 54 (`sum(byte.bit_count() for byte in bits)` over a `bytearray`:
  "internal mypyc error"); the build drops a module the compiler crashes on and compiles the rest.
- **Cython enforces the annotations it reads.** `life` fails with `TypeError: Argument 'data' has incorrect type
  (expected bytes, got bytearray)` (the same `fnv1a` annotation; plain CPython ignores it). `palindrome` fails with
  `TypeError: Expected bytearray, got bytes` at `t[2:-2:2] = text`: Cython infers `t` is a `bytearray` and its
  extended-slice assignment then wants a `bytearray` on the right. Not fixed by a directive: stock settings.
- **Numba compiles 20 of the 45 single-threaded Python kernels unchanged** (`python jitted.py --try` reruns the trial; it prints the
  first error Numba gives): `sort-int`, `sort-str`, `quicksort`, `mergesort`, `heapsort`, `radixsort`, `fannkuch`,
  `spectral-norm`, `matmul`, `dijkstra`, `edit-distance`, `knapsack`, `kmp`, `str-find`, `fib`, `binary-search`,
  `cycle`, `aho-corasick`, `window-max`, `collatz`. `spectral-norm` is decorated on the loops (`mul_atav` and what it
  calls): the outer function formats with `:.9f`, which Numba cannot compile. The rest fail on: `bytearray` (sieve,
  palindrome, life, huffman, rain-water), classes (`Body`, `Node`, `TrieNode`, `Dlx`, `XorShift32`: nbody, bst, trie,
  sudoku, hyperloglog, skiplist), `f"{x:.9f}"` (nbody), recursive tuples (binary-trees), generators in closures
  (mandelbrot, bloom), `re`, `hashlib`, `json`, `deque`, `OrderedDict`, `itemgetter`, `bisect`, `math.isqrt`
  (regex-log, sha256, json, bfs-grid, lru, kruskal, lis, pi-digits), a closure that rebinds a variable (nqueens, bwt)
  and an empty `dict` it cannot type (hashmap). The first call compiles, so the cold run and repeat 1 include
  Numba's compile; the median (repeats 2..R) is compiled code. The Python lists are converted to Numba lists and back
  on every call ("reflected lists", deprecated but still Numba's behaviour for a list argument); that is timed. Ints
  are int64 inside Numba; every kernel above stays below 2^63, and the checksums prove it.
- **Subinterpreters and processes share nothing**, so `mergesort-mt` cannot sort the caller's list in place:
  `mt_copy.py` sends each chunk to a worker, gets the sorted run back, and merges pairs on the workers in the same
  rounds as `mt.py`. The copies (pickling) are part of the time; that is the cost of the model. `mandelbrot-mt` and
  `binary-trees-mt` already return their results and run as in `mt.py`. Before timing, `pools.py` submits T 50 ms
  sleeps so all T workers exist (both executors start workers on demand); `main.py` starts all T threads of its
  thread pool the same way before timing (T tasks that wait on one barrier).
  Threads with the GIL and free-threaded threads are the existing `py314` and `py314t` rows.
- **The JIT is off unless asked**: `PYTHON_JIT=1` comes from `runtimes.json` `env`; the runtime field says `jit=on`.
- **Nuitka standalone** starts its own embedded CPython 3.14 (`runtime` says `nuitka 4.3.5 standalone`); the
  process time includes loading the `.dist` folder. Startup, size and memory of compiled programs are a separate
  suite (`suites/pyexe/`).

Windows: Nuitka, mypyc and Cython compile with MSVC (the i7 has Visual Studio 2022 17.8, MSVC 14.38); Nuitka may
download its ccache helper on first use (`--assume-yes-for-downloads`). All wheels used here ship `cp314-win_amd64`
(mypy, cython, numba, llvmlite, numpy) or are pure Python (nuitka). GraalPy:
`graalpy3.13-community-25.5.5-windows-amd64.zip` from github.com/oracle/graalpython releases, unzipped, `$GRAALPY`
pointing at its `bin/graalpy.exe`.

## 15. Autocomplete kernels (`ac-*`, group `algo`, opt-in; Short 171)

The first 10 words that start with a typed prefix, in alphabetical order (LeetCode 1268, the Unix `look` command),
answered three ways. Reference: `python/autocomplete.py`.

**Words**: `inputs/words-web2.txt`, copied to `--data` as `words-web2.txt` by `harness/data.py`: Webster's Second
International (`/usr/share/dict/words` on macOS and BSD; the 1934 copyright has lapsed), lower-cased, letters `a-z`
only, de-duplicated and sorted: 234,454 words, one per line, `\n` endings. **Shuffle** (before timing): a fresh
generator; for i from W-1 down to 1: `j = next() % (i + 1)`, swap words i and j (W = 234,454). **Queries** (the same
generator, continuing): N times `w = words[next() % W]` (shuffled order), `p = 2 + next() % 4`, query =
`w[0 .. min(p, len(w)))`. Every query has at least one match.

**fold(h, w)**: `h = h31(h, len(w)); h = h31(h, last byte of w)` (h31 step of section 2).

| kernel | `--size N` | timed work | checksum |
|---|---|---|---|
| `ac-sorted-build` | the first N shuffled words | copy the list (outside timing), then the standard sort of strings | `"{N}:{h}"`, h = fold over the words in sorted order |
| `ac-trie-build` | the same | build a trie: node = 26 child links + the index of the word ending here (-1 none); insert each word in shuffled order, the word's index into the shuffled list stored at its last node | the same as `ac-sorted-build`, from a full walk after timing (below); `extra`: `{"nodes": count incl. the root}` |
| `ac-trie-arena-build` | the same | the same trie with every node in one growable array, 32-bit child indexes (0 = none: node 0 is the root), `int32` word index | the same; `extra` the same |
| `ac-sorted` | all W words, N queries | per query: the standard lower bound of the prefix in the sorted list, then read forward while the word starts with the prefix, at most 10 | `"{N}:{results}:{h}"`, h = fold over every result of every query in order |
| `ac-trie` | the same | per query: walk the prefix's letters from the root (missing child: no results), then `collect` | the same |
| `ac-trie-arena` | the same | the same on the arena trie | the same |

The query kernels build their structure once before timing (the sorted list from the shuffled words, the tries by
inserting the shuffled words) and time only the N queries. The build kernels keep the last built structure alive
until after timing (nothing freed inside the timed region; the previous repeat's structure is freed before the next
repeat starts).

**collect(node, out, limit)** (recursive, depth at most the longest word, 24 letters): if the node holds a word,
append it; then for c = 0..25: if `len(out) >= limit` return; if child c exists, `collect(child)`. A word is listed
before the longer words below it and children go a to z, so the output is in sorted order. The full walk for the
build checksum is `collect(root, limit = W + 1)`.

Each query appends its results to one output list that is cleared per query (C++/Rust/Go/Java: a reused
vector/slice/ArrayList of string references or indexes; C: an array of `const char*`), and the hash folds the list
after the query, inside the timed region.

| | C | C++ | Rust | Go | Java | Python |
|---|---|---|---|---|---|---|
| words | `char*` array, `qsort` + `strcmp` | `std::vector<std::string>`, `std::sort` | `Vec<String>`, `sort()` | `[]string`, `slices.Sort` | `String[]`, `Arrays.sort` | `list[str]`, `list.sort()` |
| lower bound | hand-written (`strcmp`; C has none) | `std::lower_bound` | `partition_point` | `slices.BinarySearch` | `Arrays.binarySearch` (`-(i) - 1` gives the insert point) | `bisect.bisect_left` |
| prefix test | `strncmp` | `starts_with` | `starts_with` | `strings.HasPrefix` | `startsWith` | `startswith` |
| trie node | `struct { node* c[26]; int32 word; }`, `calloc` | `std::array<std::unique_ptr<Node>, 26>` + `int32` | `[Option<Box<Node>>; 26]` + `i32` | `[26]*node` + `int32` | `Node[26]` + `int` | class with `__slots__`, list of 26 |
| arena | `struct { uint32 c[26]; int32 word; }` array, doubled with `realloc` from 16 | `std::vector<Node>` | `Vec<Node>` | `[]node` + `append` | `int[]`, 27 ints per node, doubled from 16 nodes | none (as the other `-arena` rows) |

Contestable choices: the 26-slot node is the textbook trie and the same in every language (a `dict` of children is
the more common Python idiom, as in `trie`); results are alphabetical, not ranked by popularity (a ranked
typeahead keeps a top-k list in every trie node, which this row does not test); the sorted list is built once and
never changes (a trie takes inserts without re-sorting).

Sizes: build kernels full 234,454 (all words), quick 20,000, probe 234,454; query kernels full 200,000 (CPython's trie takes
about 10-20 s per run there), quick 20,000, probe 50,000.

**C++ trie variants (Short 171, where a trie query's time goes).** Same words, queries and checksums as `ac-trie`:
`ac-trie-sorted` builds the pointer trie from the words in A to Z order (nodes are allocated in the order the walk
reads them); `ac-trie-mask` and `ac-trie-sorted-mask` add a 26-bit mask of filled slots to every node (it sits in the
node's padding, so the node stays 216 bytes) and `collect` visits only the set bits, lowest first (same a to z
order, same results); `ac-trie-dfs` copies the arena trie into a new array in walk order (parent, then children a to
z), node = 26 `uint32` links + `int32` word + `uint32` mask (112 bytes); `ac-trie-dfs-mask` is that array walked
through the mask. The copy and the masks are made before timing.

Every C++ `ac-sorted`/`ac-trie*` query kernel also reports in `extra`, measured after the timed repeats (the
harness's `times_ms` is unchanged), each the median over the repeats: `prefix_ms`, a pass that only finds each
query's start (the lower bound, or the walk down to the prefix node); `collect_ms`, a pass that only lists the
first ten matches from starts found beforehand; `twice_ms`, a pass that runs every query twice in a row (the second
run finds the query's nodes in cache). Trie kernels add `nodes_per_query` (nodes `collect` visits, the prefix node
included) and `slots_per_query` (child slots it reads: every slot up to the tenth result, or set mask bits only).
`sink` keeps the passes from being optimised away.

## 16. Small-array search kernels (`ssearch.LANG.WAY`, group `algo`, opt-in; Short 170)

When does a plain loop beat binary search? Every kernel sweeps the same 17 array sizes, `n` = 4, 6, 8, 12, 16, 24,
32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024 (powers of two and the sizes between them: a binary search over
a power of two always takes the same number of steps, which flatters it). For each size, with a fresh generator:
`a[i] = a[i-1] + 1 + next() % 4` (a[-1] = 0, as `binary-search`; distinct, sorted), then `--size` queries
`next() % (a[n-1] + 1)`. Each query asks for the lower bound (the first i with `a[i] >= q`, n if none).

Each repeat runs every size in turn and times each size's query loop alone; `times_ms` is the sum over the 17 sizes,
and `extra` holds `ns_<n>`, the median over the repeats of that size's nanoseconds per query. The chart reads
`extra`; `times_ms` only keeps the harness contract. Checksum: `"{hits}:{sum of lower bounds}"` summed over all
sizes (hits: `a[lo] == q`). Every way of every language prints the same checksum.

| way | what it is | C | C++ | Rust | Go | Python |
|---|---|---|---|---|---|---|
| `linear` | the plain loop: stop at the first `a[i] >= q` | `for` | `for` | `iter().position` | `for range` | `for i, x in enumerate(a)` |
| `linear-count` | count `a[i] < q` over the whole array: no early exit, so compilers can vectorise it | `for` | range `for` | `iter().filter().count()` | `for range` | - |
| `binary` | hand-written lower bound, `mid = (lo + hi) / 2` (the code of `binary-search`) | yes | yes | yes | yes | yes |
| built-in | the standard library call | - (C has `bsearch`, which finds an equal key, not a lower bound) | `std::lower_bound` | `binary_search` (Err/Ok index; values are distinct) and `partition_point` (calls `binary_search_by`) | `sort.Search` and `slices.BinarySearch` | `bisect.bisect_left` (C code) |
| `branchless` | halve `len` every step and move `base` with a conditional select, no data-dependent branch (the branch-free search of Khuong and Morin, "Array Layouts for Comparison-Based Searching", 2017) | yes | yes | - (`binary_search` and `partition_point` are already written this way: no early exit, `hint::select_unpredictable`) | - | - |

C, C++ and Rust call the search through a function the compiler can inline (a macro-generated loop per way in C, a
template in C++, a generic `impl Fn` in Rust); Go writes one query loop per way with a direct call. Python calls a
plain function per query. Sizes: full 100,000 queries per size (Python 10,000: its `linear` row takes about 2 s per
repeat on the i7), quick 2,000, probe 20,000 (Python 2,000).

Go timing: Go's clock on Windows moves in steps of about 0.5 ms, and one pass of 100,000 queries takes only a few
milliseconds, so each Go sample runs the size's query loop pass after pass until 64 ms have passed (one clock step is
under 1% of the sample) and divides by passes x queries. `times_ms` is then one pass per size, summed; the checksum
is that of one pass. C, C++ and Rust read QPC-backed clocks and time one pass.

Repeated-query controls (Go only): `ssearch.go.WAY.same` for `linear`, `binary`, `sort.Search` and
`slices.BinarySearch` run the same arrays and the same code, but every one of the `--size` queries is `queries[2]` of
that size (the third random query). Every lookup then takes the same path, so the CPU's branch guesses come true; the
gap to the random-query row is what mispredicted jumps cost. Checksum by the same rule, so it differs from the random
rows: `14000:5232000` at quick, `140000:52320000` at probe, `700000:261600000` at full.

Contestable choices: queries are random, so the plain loop stops on average halfway; a workload where most queries
fall near the front (or the end) moves the crossover. The values are 64-bit; 32-bit keys fit twice as many in a SIMD
register. The arrays sit in L1 cache, as small arrays in real code usually do.
