# Race: mergesort-mt (python)
# Extracted from suite/python/mt.py — build and run via suite/run.py

from concurrent.futures import ThreadPoolExecutor
from sorts import merge, merge_sort


def chunk_bounds(n: int, threads: int) -> list[int]:
    return [t * n // threads for t in range(threads + 1)]


def mergesort_mt(a: list[int], threads: int, pool: ThreadPoolExecutor) -> list[int]:
    aux = [0] * len(a)
    bounds = chunk_bounds(len(a), threads)
    jobs = [pool.submit(merge_sort, a, aux, bounds[t], bounds[t + 1]) for t in range(threads)]
    for job in jobs:
        job.result()
    while len(bounds) > 2:
        jobs = [
            pool.submit(merge, a, aux, bounds[p], bounds[p + 1], bounds[p + 2]) for p in range(0, len(bounds) - 1, 2)
        ]
        for job in jobs:
            job.result()
        bounds = bounds[::2]
    return a
