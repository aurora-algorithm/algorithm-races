"""main.py with Numba's @njit on a kernel's own functions, the source otherwise unchanged (kernels/SPEC.md section 14).

usage: python jitted.py KERNEL --size N ...   (only the kernels in NJIT; the others exit 2)
       python jitted.py --try                  try every kernel at its quick size and print which ones qualify

Wrapping a function in numba.njit here is the same as writing @numba.njit above it. A kernel's entry names its
functions; the functions they call in the same module are wrapped too, as the decorator would have to be on them.
A kernel qualifies when every one of them compiles in nopython mode and the checksum matches expected.json.
Numba converts the Python list arguments to its own lists on every call (reflected lists); that is in the timing.
"""

import importlib
import json
import subprocess
import sys
import types
import warnings
from pathlib import Path

HERE = Path(__file__).resolve().parent

# kernel -> (module, functions it runs); every kernel the Python reference has, so --try covers them all
CANDIDATES: dict[str, tuple[str, tuple[str, ...]]] = {
    "sort-int": ("sorts", ("run_builtin",)),
    "sort-str": ("sorts", ("run_builtin",)),
    "quicksort": ("sorts", ("run_quicksort",)),
    "mergesort": ("sorts", ("run_mergesort",)),
    "heapsort": ("sorts", ("run_heapsort",)),
    "radixsort": ("sorts", ("radix_sort",)),
    "binary-trees": ("bg", ("binary_trees",)),
    "nbody": ("bg", ("nbody",)),
    "mandelbrot": ("bg", ("mandelbrot",)),
    "fannkuch": ("bg", ("fannkuch",)),
    # spectral_norm formats its result with :.9f (no Numba support), so the decorator goes on the loops it calls
    "spectral-norm": ("bg", ("mul_atav",)),
    "regex-log": ("misc", ("regex_log",)),
    "hashmap": ("misc", ("hashmap",)),
    "sha256": ("misc", ("sha256",)),
    "sieve": ("misc", ("sieve",)),
    "matmul": ("misc", ("matmul",)),
    "json": ("misc", ("json_roundtrip",)),
    "bfs-grid": ("algo", ("bfs_grid",)),
    "dijkstra": ("algo", ("dijkstra",)),
    "kruskal": ("algo", ("kruskal",)),
    "bst": ("algo", ("bst",)),
    "trie": ("algo", ("trie",)),
    "lru": ("algo", ("lru",)),
    "edit-distance": ("algo", ("edit_distance",)),
    "knapsack": ("algo", ("knapsack",)),
    "lis": ("algo", ("lis",)),
    "kmp": ("algo", ("kmp",)),
    "str-find": ("algo", ("str_find",)),
    "fib": ("algo", ("fib",)),
    "nqueens": ("algo", ("nqueens",)),
    "binary-search": ("algo", ("binary_search",)),
    "sudoku": ("puzzle", ("sudoku",)),
    "bloom": ("puzzle", ("bloom",)),
    "hyperloglog": ("puzzle", ("hyperloglog",)),
    "skiplist": ("puzzle", ("skiplist",)),
    "cycle": ("puzzle", ("cycle",)),
    "palindrome": ("puzzle", ("palindrome",)),
    "bwt": ("puzzle", ("bwt",)),
    "aho-corasick": ("puzzle", ("aho_corasick",)),
    "pi-digits": ("puzzle", ("pi_digits",)),
    "life": ("puzzle", ("life",)),
    "huffman": ("puzzle", ("huffman",)),
    "window-max": ("puzzle", ("window_max",)),
    "rain-water": ("puzzle", ("rain_water",)),
    "collatz": ("puzzle", ("collatz",)),
}

# the kernels that passed `jitted.py --try` (kernels/SPEC.md section 14 lists them and why the others fail)
NJIT = (
    "sort-int",
    "sort-str",
    "quicksort",
    "mergesort",
    "heapsort",
    "radixsort",
    "fannkuch",
    "spectral-norm",
    "matmul",
    "dijkstra",
    "edit-distance",
    "knapsack",
    "kmp",
    "str-find",
    "fib",
    "binary-search",
    "cycle",
    "aho-corasick",
    "window-max",
    "collatz",
)


def callees(module: types.ModuleType, name: str, seen: set[str]) -> None:
    """`name` and the plain functions of `module`'s namespace that it calls, transitively."""
    fn = getattr(module, name)
    if name in seen or not isinstance(fn, types.FunctionType):
        return
    seen.add(name)
    for code in [fn.__code__, *(c for c in fn.__code__.co_consts if isinstance(c, types.CodeType))]:
        for called in code.co_names:
            if isinstance(getattr(module, called, None), types.FunctionType):
                callees(module, called, seen)


def apply_njit(kernel: str) -> list[str]:
    import numba

    module_name, entries = CANDIDATES[kernel]
    module = importlib.import_module(module_name)
    names: set[str] = set()
    for entry in entries:
        callees(module, entry, names)
    for name in sorted(names):
        setattr(module, name, numba.njit(getattr(module, name)))
    return [f"{module_name}.{n}" for n in sorted(names)]


def run(argv: list[str], allowed: tuple[str, ...]) -> int:
    if not argv or argv[0] not in allowed:
        print(f"unknown kernel {argv[0] if argv else ''} (Numba runs: {' '.join(allowed)})", file=sys.stderr)
        return 2
    import numba

    # reflected-list deprecation notices go to stderr on every run; the harness only reads stdout
    warnings.simplefilter("ignore", numba.NumbaPendingDeprecationWarning)
    wrapped = apply_njit(argv[0])
    import main as driver

    plain = driver.runtime_info
    info = f"numba {numba.__version__} njit: {' '.join(wrapped)}"
    driver.runtime_info = lambda use_numpy, kernel: f"{plain(use_numpy, kernel)} {info}"
    sys.argv = [sys.argv[0], *argv]
    return driver.main()


def failure(stderr: str) -> str:
    """Numba's first explanation line (after its 'Failed in nopython mode pipeline' header), else the last line."""
    lines = [line.strip() for line in stderr.splitlines() if line.strip() and "RuntimeWarning" not in line]
    header = "Failed in nopython mode pipeline"
    starts = [i for i, line in enumerate(lines) if header in line]
    after = [line for line in lines[starts[0] :] if header not in line] if starts else []
    return (after or lines or ["no output"])[0 if after else -1][:160]


def try_all() -> int:
    root = HERE.parent
    table = json.loads((root / "kernels.json").read_text())
    expected = json.loads((root / "expected.json").read_text())
    sys.path.insert(0, str(root))
    from harness import data

    passed = []
    for kernel in CANDIDATES:
        size = table[kernel]["quick"]
        data.ensure(kernel, table[kernel], size)
        argv = [sys.executable, __file__, "--trial", kernel, "--size", str(size), "--repeats", "2"]
        r = subprocess.run([*argv, "--data", str(data.DATA)], cwd=HERE, capture_output=True, text=True)
        try:
            got = json.loads(r.stdout.strip().splitlines()[-1])["checksum"]
        except (IndexError, ValueError, KeyError):
            got = None
        ok = got is not None and got == expected.get(f"{kernel}@{size}")
        why = "" if ok else failure(r.stderr)
        print(f"{'ok  ' if ok else 'FAIL'} {kernel:14} {why}", flush=True)
        if ok:
            passed.append(kernel)
    print("qualify:", json.dumps(passed))
    return 0


def main() -> int:
    if sys.argv[1:2] == ["--try"]:
        return try_all()
    if sys.argv[1:2] == ["--trial"]:
        return run(sys.argv[2:], tuple(CANDIDATES))
    return run(sys.argv[1:], NJIT)


if __name__ == "__main__":
    sys.exit(main())
