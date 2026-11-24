# algorithm-races

The source code, harness, and measured results behind the **Algorithm Races** benchmark series across **C, C++, Rust, Go, Goose, C#, Java, JavaScript, and Python**.

Each race has its own folder in [`races/`](races/) containing **all language files for that race** side by side (`c.c`, `cpp.cpp`, `rust.rs`, `go.go`, `goose.goose`, `csharp.cs`, `java.java`, `js.mjs`, `python.py`) plus its measured results table and run commands.

Every language implements the exact same algorithm on the exact same generated input and prints an identical checksum verified against [`suite/expected.json`](suite/expected.json). All published numbers are medians of warm repeats on an **Intel Core i7-13700F, Windows 11, stock settings**, pinned to one CPU core (except the `-mt` parallel kernels, which run on 8 threads).

- **Setup & running on Windows, macOS, and Linux:** [docs/SETUP.md](docs/SETUP.md)
- **Timing rules, CPU pinning, warm repeats, and stock-settings policy:** [docs/METHODOLOGY.md](docs/METHODOLOGY.md)
- **Exact input generators, checksums, and algorithm specifications:** [SPEC.md](SPEC.md)

## Quick start

```bash
git clone https://github.com/aurora-algorithm/algorithm-races.git
cd algorithm-races

# Build only the runtimes you have installed (Python 3.11+ stdlib only for the runner)
python3 suite/run.py build --runtime rust --runtime go --runtime node --runtime py314

# Fast smoke test + checksum check against suite/expected.json
python3 suite/run.py bench --quick --kernel mergesort --runtime rust --runtime go --runtime node --runtime py314

# Full benchmark for one race across all built runtimes
python3 suite/run.py bench --kernel mergesort
```

See [docs/SETUP.md](docs/SETUP.md) for OS-specific installation (Windows, macOS, Linux), C/C++/Java/C#/Goose setup, environment variables, and troubleshooting.

## Races

| Race | Workload | Fastest | Slowest |
| :--- | :--- | :--- | :--- |
| [0/1 knapsack](races/knapsack/) | 1,000 items and a bag of 200,000 | Java on HotSpot 25 (39 ms) | Python 3.14 (9,205 ms) |
| [Aho-Corasick](races/aho-corasick/) | 10,000 patterns in 50,331,648 letters of DNA | C++ with GCC 16 (197 ms) | Python 3.14 (2,657 ms) |
| [Argon2id password hashing](races/argon2id/) | 32 passwords at 19 MiB each | Rust 1.99 (401 ms) | C# on .NET 10 JIT (1,050 ms) |
| [Bcrypt](races/bcrypt/) | 32 passwords at cost 10 | C with GCC 16 (1,178 ms) | C# on .NET 10 JIT (1,558 ms) |
| [Binary search](races/binary-search/) | 2,000,000 lookups in 2,000,000 sorted numbers | Goose 2026.4 · MSVC (197 ms) | Python 3.14 (3,072 ms) |
| [Binary search tree](races/bst/) | 1,000,000 keys | Java on HotSpot 25 (496 ms) | Python 3.14 (3,561 ms) |
| [Binary search tree in one array](races/bst-arena/) | 1,000,000 keys | C++ with GCC 16 (426 ms) | JavaScript on Node 24 (647 ms) |
| [Binary trees](races/binary-trees/) | the depth 21 run | Java on HotSpot 25 (2,649 ms) | Python 3.14 (42,293 ms) |
| [Binary trees on 8 threads](races/binary-trees-mt/) | the depth 21 run | Java on HotSpot 25 (665 ms) | Python 3.14 (45,810 ms) |
| [Binary trees with an arena](races/binary-trees-arena/) | the depth 21 run | Goose 2026.4 · MSVC (1,210 ms) | JavaScript on Node 24 (7,182 ms) |
| [Bloom filter](races/bloom/) | 2,000,000 keys and 2,000,000 lookups | C++ with GCC 16 (55 ms) | Python 3.14 (5,104 ms) |
| [Breadth-first search](races/bfs-grid/) | a 5,000 × 5,000 maze | C++ with GCC 16 (339 ms) | Python 3.14 (8,538 ms) |
| [Built-in sort](races/sort-int/) | 10,000,000 numbers | Goose with sort (166 ms) | JavaScript on Node 24 · sort (3,426 ms) |
| [Burrows-Wheeler transform](races/bwt/) | 1,000,000 letters of DNA | Rust 1.99 (192 ms) | Python 3.14 (2,920 ms) |
| [Collatz conjecture](races/collatz/) | 20,000,000 starts | Rust 1.99 (98 ms) | Python 3.14 (6,055 ms) |
| [Conway's Game of Life](races/life/) | 512 × 512 cells, 100 generations | C++ with GCC 16 (70 ms) | Python 3.14 (6,289 ms) |
| [CSV parsing](races/csv/) | a 1 GB CSV | C# on .NET 10 JIT (664 ms) | JavaScript on Node 24 (49,615 ms) |
| [Dijkstra's algorithm](races/dijkstra/) | 2,000,000 nodes | Goose 2026.4 · MSVC (400 ms) | Python 3.14 (7,706 ms) |
| [Edit distance](races/edit-distance/) | a 10,000 x 10,000 grid | C++ with GCC 16 (62 ms) | Python 3.14 (10,172 ms) |
| [Fannkuch](races/fannkuch/) | an 11-pancake stack | C with GCC 16 (1,511 ms) | Python 3.14 (43,726 ms) |
| [Floyd's cycle detection](races/cycle/) | 4,000 starts mod 67,108,859 | Rust 1.99 (314 ms) | Python 3.14 (7,712 ms) |
| [Gzip compression](races/gzip/) | 100 MiB of logs, JSON and code | Go 1.27 (555 ms) | Rust 1.99 (1,890 ms) |
| [Gzip decompression](races/gunzip/) | 100 MiB of logs, JSON and code | Python 3.14 (118 ms) | Go 1.27 (249 ms) |
| [Hash map](races/hashmap/) | 10,000,000 keys | Goose 2026.4 · MSVC (727 ms) | JavaScript on Node 24 (5,811 ms) |
| [Hash map with number keys](races/bmap-int/) | 10,000,000 numbers | Goose with dictionary (763 ms) | JavaScript on Node 24 · Map (5,792 ms) |
| [Hash map with string keys](races/bmap-str/) | 1,000,000 strings | Goose with dictionary (87 ms) | JavaScript on Node 24 · Map (338 ms) |
| [Heapsort](races/heapsort/) | 10,000,000 numbers | Java on HotSpot 25 (1,459 ms) | Python 3.14 (41,910 ms) |
| [Huffman coding](races/huffman/) | 16 MiB of text | C++ with GCC 16 (256 ms) | Python 3.14 (6,027 ms) |
| [HyperLogLog](races/hyperloglog/) | 12,000,000 items | C# on .NET 10 JIT (22 ms) | Python 3.14 (6,722 ms) |
| [Image resizing](races/image/) | 8 photos of 6,000 × 4,000 | JavaScript with sharp (533 ms) | Go with image/jpeg (4,992 ms) |
| [JSON parsing](races/json/) | 450,000 orders, 45.5 MB | Goose 2026.4 · MSVC (90 ms) | C++ with GCC 16 (1,026 ms) |
| [KMP string search](races/kmp/) | 50,000,000 letters of DNA | C with GCC 16 (122 ms) | Python 3.14 (2,485 ms) |
| [Kruskal's algorithm](races/kruskal/) | 2,000,000 points | Rust 1.99 (296 ms) | Python 3.14 (6,558 ms) |
| [Longest increasing subsequence](races/lis/) | 10,000,000 numbers | Rust 1.99 (89 ms) | Python 3.14 (1,538 ms) |
| [LRU cache](races/lru/) | a cache of 100,000 | Java on HotSpot 25 (302 ms) | Python 3.14 (2,169 ms) |
| [Manacher's algorithm](races/palindrome/) | a string of 12,000,000 letters | C# on .NET 10 JIT (152 ms) | Python 3.14 (6,247 ms) |
| [Mandelbrot fractal](races/mandelbrot-mt/) | a 4,000 × 4,000 picture | C with GCC 16 (83 ms) | Python 3.14 (27,810 ms) |
| [Mandelbrot set](races/mandelbrot/) | a 4,000 × 4,000 picture | Java on HotSpot 25 (660 ms) | Python 3.14 (27,964 ms) |
| [Matrix multiplication](races/matmul/) | two 1,024 × 1,024 matrices | Python with NumPy 2.5 (30 ms) | C++ with GCC 16 (2,608 ms) |
| [Merge sort](races/mergesort/) | 10,000,000 numbers | C with GCC 16 (792 ms) | Python 3.14 (21,712 ms) |
| [Merge sort](races/mergesort-mt/) | 10,000,000 numbers | C with GCC 16 (152 ms) | Python 3.14 (23,496 ms) |
| [N-body simulation](races/nbody/) | 5,000,000 steps | Rust 1.99 (147 ms) | Python 3.14 (13,888 ms) |
| [N-Queens backtracking](races/nqueens/) | 13 queens on a 13 × 13 board | Rust 1.99 (179 ms) | Python 3.14 (2,158 ms) |
| [Ordered map](races/ordered-map/) | 1,000,000 keys | Rust 1.99 (252 ms) | C# on .NET 10 JIT (1,038 ms) |
| [Photo shrink with other libraries](races/image-alt/) | 8 photos of 6,000 × 4,000 pixels | Python with pyvips (631 ms) | JavaScript with Node · jimp (20,248 ms) |
| [Pi digits](races/pi-digits/) | 500,000 digits | JavaScript on Node 24 (304 ms) | C++ with Boost, GCC 16 (6,878 ms) |
| [Quicksort](races/quicksort/) | 10,000,000 numbers | Go 1.27 (644 ms) | Python 3.14 (13,968 ms) |
| [Radix sort](races/bsort-radix/) | 10,000,000 numbers below 2^32 held as 64-bit integers | Go with zermelo (101 ms) | JavaScript with Array.sort (3,410 ms) |
| [Radix sort](races/radixsort/) | 10,000,000 32-bit numbers | Goose 2026.4 · MSVC (73 ms) | Python 3.14 (7,754 ms) |
| [Recursive Fibonacci](races/fib/) | fib(40): 331 million calls | C++ with GCC 16 (85 ms) | Python 3.14 (8,570 ms) |
| [Regex log search](races/regex-log/) | 2,000,000 log lines | JavaScript on Node 24 (48 ms) | C++ with GCC 16 (2,308 ms) |
| [Regex log search](races/regex-log-alt/) | a 70 MB log of 2,000,000 lines | JavaScript on Node 24 (48 ms) | Go 1.27 (1,865 ms) |
| [SHA-256 hashing](races/sha256/) | 1 GiB of data in memory | JavaScript on Node 24 (437 ms) | C with GCC 16 (2,353 ms) |
| [Sieve of Eratosthenes](races/sieve/) | the numbers up to 100,000,000 | C++ with GCC 16 (489 ms) | Python 3.14 (5,700 ms) |
| [Skip list](races/skiplist/) | 1,000,000 keys | C with GCC 16 (676 ms) | Python 3.14 (11,978 ms) |
| [Sliding window maximum](races/window-max/) | 8,000,000 values, 5 window sizes | C++ with GCC 16 (263 ms) | Python 3.14 (8,711 ms) |
| [Sorting a backwards list](races/bsort-str-reversed/) | 1,000,000 words of 8 to 16 letters in Z to A order | Rust with sort (4.4 ms) | C# with Array.Sort (178 ms) |
| [Sorting numbers already in order](races/bsort-int-sorted/) | 10,000,000 numbers already in order | Java with Arrays.sort (4.0 ms) | C# with Array.Sort (362 ms) |
| [Sorting numbers in reverse order](races/bsort-int-reversed/) | 10,000,000 numbers from biggest to smallest | Java with Arrays.sort (8.2 ms) | Python with list.sort (367 ms) |
| [Sorting words](races/sort-str/) | 1,000,000 words | Rust with sort (121 ms) | JavaScript on Node 24 · sort (403 ms) |
| [Sorting words already in order](races/bsort-str-sorted/) | 1,000,000 words already in order | Rust with sort (3.2 ms) | C with qsort (107 ms) |
| [Spectral norm](races/spectral-norm/) | a 3,000 × 3,000 matrix | C with GCC 16 (284 ms) | Python 3.14 (33,505 ms) |
| [String join](races/bjoin/) | 10,000,000 parts | Goose with append (41 ms) | JavaScript with Node · Array.join (175 ms) |
| [String search](races/str-find/) | 50,000,000 letters of DNA | Go 1.27 (6.5 ms) | C++ with GCC 16 (163 ms) |
| [Sudoku solver](races/sudoku/) | 120 sudokus of 26 clues | C with GCC 16 (150 ms) | Python 3.14 (6,523 ms) |
| [Trapping rain water](races/rain-water/) | a 1,500 × 1,500 map | Rust 1.99 (163 ms) | Python 3.14 (2,714 ms) |
| [Trie](races/trie/) | 500,000 words and 500,000 prefixes | Go 1.27 (247 ms) | Python 3.14 (1,973 ms) |
| [Trie in one array](races/trie-arena/) | 500,000 words | Goose 2026.4 · MSVC (28 ms) | Go 1.27 (210 ms) |
| [Word count](races/wordcount/) | a 1 GiB text file | Goose 2026.4 · MSVC (3,019 ms) | JavaScript on Node 24 (20,346 ms) |
| [Zstd compression](races/zstd/) | 100 MiB of logs, JSON and code | C with GCC 16 (200 ms) | C# on .NET 10 JIT (437 ms) |
| [Zstd decompression](races/unzstd/) | 100 MiB of logs, JSON and code | Go 1.27 (60 ms) | C# on .NET 10 JIT (177 ms) |
