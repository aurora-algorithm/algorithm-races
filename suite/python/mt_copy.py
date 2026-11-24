"""mergesort-mt for workers that share no memory (pools.py): the same chunks and merge rounds as mt.py, but each
chunk is copied to a worker and the sorted run copied back, because a subinterpreter or a process cannot sort the
caller's list in place."""

from concurrent.futures import Executor

from mt import chunk_bounds
from sorts import merge, run_mergesort


def merged(left: list[int], right: list[int]) -> list[int]:
    a = left + right
    merge(a, [0] * len(a), 0, len(left), len(a))
    return a


def mergesort_mt(a: list[int], threads: int, pool: Executor) -> list[int]:
    bounds = chunk_bounds(len(a), threads)
    runs = list(pool.map(run_mergesort, [a[bounds[t] : bounds[t + 1]] for t in range(threads)]))
    while len(runs) > 1:
        runs = list(pool.map(merged, runs[0::2], runs[1::2]))
    a[:] = runs[0]
    return a
