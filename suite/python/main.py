"""Kernel runner for Python (kernels/SPEC.md section 7).

usage: python main.py KERNEL --size N [--repeats R] [--threads T] [--data DIR] [--numpy]
Prints one JSON line: kernel, size, threads, checksum, times_ms (one per repeat, kernel only).
"""

import argparse
import json
import sys
import threading
import time
from concurrent.futures import Executor, ThreadPoolExecutor
from pathlib import Path

import algo
import autocomplete
import bg
import misc
import mt
import puzzle
import sorts
import ssearch
import ways
import words
import ways_libs
import work
from gen import XorShift32, fnv1a, h31, log_text, matrices, random_strings, sha_buffer


def sorted_ints(a) -> str:
    return str(h31(a))


def strings_checksum(a: list[str]) -> str:
    return str(fnv1a("".join(s + "\n" for s in a).encode()))


def bytes_checksum(b: bytes) -> str:
    return str(fnv1a(b))


def text_checksum(s: str) -> str:
    return str(fnv1a(s.encode()))


def hashmap_input(n: int):
    inserts = XorShift32().ints(n)
    return inserts, [k ^ (i & 1) for i, k in enumerate(inserts)]


def ints_input(n: int, _data: Path):
    return XorShift32().ints(n)


def sort_kernel(fn):
    return {"prepare": ints_input, "fresh": list.copy, "run": lambda a, _t, _p: fn(a), "check": sorted_ints}


def kernels() -> dict:
    return {
        "sort-int": sort_kernel(sorts.run_builtin),
        "sort-str": {
            "prepare": lambda n, _d: random_strings(n),
            "fresh": list.copy,
            "run": lambda a, _t, _p: sorts.run_builtin(a),
            "check": strings_checksum,
        },
        "quicksort": sort_kernel(sorts.run_quicksort),
        "mergesort": sort_kernel(sorts.run_mergesort),
        "heapsort": sort_kernel(sorts.run_heapsort),
        "radixsort": sort_kernel(sorts.radix_sort),
        "binary-trees": {"run": lambda n, _t, _p: bg.binary_trees(n), "check": text_checksum},
        "nbody": {"run": lambda n, _t, _p: bg.nbody(n), "check": str},
        "mandelbrot": {"run": lambda n, _t, _p: bg.mandelbrot(n), "check": bytes_checksum},
        "fannkuch": {"run": lambda n, _t, _p: bg.fannkuch(n), "check": str},
        "spectral-norm": {"run": lambda n, _t, _p: bg.spectral_norm(n), "check": str},
        "regex-log": {"prepare": lambda n, _d: log_text(n), "run": lambda s, _t, _p: misc.regex_log(s), "check": str},
        "hashmap": {
            "prepare": lambda n, _d: hashmap_input(n),
            "run": lambda kv, _t, _p: misc.hashmap(*kv),
            "check": str,
        },
        "sha256": {"prepare": lambda n, _d: sha_buffer(n), "run": lambda b, _t, _p: misc.sha256(b), "check": str},
        "sieve": {"run": lambda n, _t, _p: misc.sieve(n), "check": str},
        "matmul": {
            "prepare": lambda n, _d: (*matrices(n), n),
            "run": lambda m, _t, _p: misc.matmul(*m),
            "check": misc.matmul_checksum,
        },
        "json": {
            "prepare": lambda n, d: (d / f"orders-{n}.json").read_text(encoding="utf-8"),
            "run": lambda s, _t, _p: misc.json_roundtrip(s),
            "check": misc.json_checksum,
        },
        "mergesort-mt": {
            "pool": True,
            "prepare": ints_input,
            "fresh": list.copy,
            "run": mt.mergesort_mt,
            "check": sorted_ints,
        },
        "mandelbrot-mt": {"pool": True, "run": mt.mandelbrot_mt, "check": bytes_checksum},
        "binary-trees-mt": {"pool": True, "run": mt.binary_trees_mt, "check": text_checksum},
    }


def numpy_kernels() -> dict:
    import numpy as np

    def np_matrices(n: int):
        a, b = matrices(n)
        return np.array(a).reshape(n, n), np.array(b).reshape(n, n)

    def np_sort(a):
        a.sort()
        return a

    return {
        "sort-int": {
            "prepare": lambda n, _d: np.array(XorShift32().ints(n), dtype=np.int64),
            "fresh": np.copy,
            "run": lambda a, _t, _p: np_sort(a),
            "check": lambda a: sorted_ints(a.tolist()),
        },
        "matmul": {
            "prepare": lambda n, _d: np_matrices(n),
            "run": lambda m, _t, _p: m[0] @ m[1],
            "check": lambda c: misc.matmul_checksum(c.ravel().tolist()),
        },
    }


def runtime_info(use_numpy: bool, kernel: str) -> str:
    gil = getattr(sys, "_is_gil_enabled", lambda: True)()
    info = f"{sys.implementation.name} {sys.version.split()[0]} gil={'on' if gil else 'off'}"
    if kernel in work.kernels(1):
        info += f" {work.libraries()}"
    if ".ndarray." in kernel or ".polars." in kernel:
        info += f" {ways_libs.libraries()}"
    if use_numpy:
        import numpy as np

        info += f" numpy {np.__version__}"
    return info


def start_pool(workers: int) -> Executor:
    """All T threads exist before the timer: the pool starts a thread per task while none is idle, and the barrier
    keeps every task busy until T have arrived (pools.py starts its workers the same way)."""
    pool = ThreadPoolExecutor(max_workers=workers)
    barrier = threading.Barrier(workers)
    for job in [pool.submit(barrier.wait) for _ in range(workers)]:
        job.result()
    return pool


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("kernel")
    ap.add_argument("--size", type=int, required=True)
    ap.add_argument("--repeats", type=int, default=1)
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--data", type=Path, default=Path("."))
    ap.add_argument("--numpy", action="store_true")
    args = ap.parse_args()
    if args.numpy:
        table = numpy_kernels()
    else:
        table = kernels() | work.kernels(args.threads) | algo.kernels() | puzzle.kernels() | autocomplete.kernels()
        table |= ways.kernels() | ways_libs.kernels(args.threads) | words.kernels() | ssearch.kernels()
    if args.kernel not in table:
        print(f"unknown kernel {args.kernel}", file=sys.stderr)
        return 2
    k = table[args.kernel]
    prepare = k.get("prepare", lambda n, _d: n)
    fresh = k.get("fresh", lambda x: x)
    base = prepare(args.size, args.data)
    pool = start_pool(args.threads) if k.get("pool") else ThreadPoolExecutor(max_workers=args.threads)
    times = []
    result = None
    for _ in range(args.repeats):
        item = fresh(base)
        t0 = time.perf_counter_ns()
        result = k["run"](item, args.threads, pool)
        times.append(round((time.perf_counter_ns() - t0) / 1e6, 3))
    pool.shutdown()
    checksum = k["check"](result)
    extra = None
    if isinstance(checksum, tuple):
        checksum, extra = checksum
    row = {
        "kernel": args.kernel,
        "size": args.size,
        "threads": args.threads,
        "checksum": checksum,
        "times_ms": times,
        "runtime": runtime_info(args.numpy, args.kernel),
    }
    if extra:
        row["extra"] = extra
    print(json.dumps(row))
    return 0


if __name__ == "__main__":
    sys.exit(main())
