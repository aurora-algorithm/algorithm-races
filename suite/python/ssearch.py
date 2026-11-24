"""The ssearch kernels (../SPEC.md section 16): lower bound in small sorted arrays, a plain loop against binary
search, at every size from 4 to 1024. Kernel names are ssearch.python.WAY."""

import bisect
import statistics
import time
from collections.abc import Callable

from gen import XorShift32

SIZES = (4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024)


def linear(a: list[int], q: int) -> int:
    for i, x in enumerate(a):
        if x >= q:
            return i
    return len(a)


def binary(a: list[int], q: int) -> int:
    lo, hi = 0, len(a)
    while lo < hi:
        mid = (lo + hi) // 2
        if a[mid] < q:
            lo = mid + 1
        else:
            hi = mid
    return lo


def inputs(q_count: int) -> list[tuple[list[int], list[int]]]:
    out = []
    for n in SIZES:
        rng = XorShift32()
        a, v = [], 0
        for _ in range(n):
            v += 1 + rng.next() % 4
            a.append(v)
        out.append((a, [rng.next() % (v + 1) for _ in range(q_count)]))
    return out


def sweep(find: Callable[[list[int], int], int]) -> Callable:
    """One repeat: every size in turn, each size's query loop timed alone; the per-size times of every repeat are
    kept on the prepared input so check() can report their medians."""

    def run(prepared: dict, _threads: int, _pool) -> dict:
        hits = total = 0
        for (a, queries), ns in zip(prepared["sizes"], prepared["ns"], strict=True):
            n = len(a)
            h = t = 0
            t0 = time.perf_counter_ns()
            for q in queries:
                lo = find(a, q)
                if lo < n and a[lo] == q:
                    h += 1
                t += lo
            ns.append((time.perf_counter_ns() - t0) / len(queries))
            hits += h
            total += t
        prepared["checksum"] = f"{hits}:{total}"
        return prepared

    return run


def check(prepared: dict) -> tuple[str, dict]:
    extra = {f"ns_{n}": round(statistics.median(ns), 3) for n, ns in zip(SIZES, prepared["ns"], strict=True)}
    return prepared["checksum"], extra


def prepare(q_count: int, _data) -> dict:
    return {"sizes": inputs(q_count), "ns": [[] for _ in SIZES]}


def kernels() -> dict:
    ways = {"linear": linear, "binary": binary, "bisect_left": bisect.bisect_left}
    return {
        f"ssearch.python.{way}": {"prepare": prepare, "run": sweep(find), "check": check} for way, find in ways.items()
    }
