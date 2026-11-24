"""Multi-threaded kernels (kernels/SPEC.md section 6) on a thread pool made before timing. With the GIL (every CPython
build except 3.14t) the threads take turns, so these show what threads buy each runtime as-is."""

from concurrent.futures import ThreadPoolExecutor

from bg import binary_trees_lines, depth_checks, mandelbrot_row
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


def mandelbrot_rows(n: int, first: int, step: int) -> list[bytes]:
    return [mandelbrot_row(n, y) for y in range(first, n, step)]


def mandelbrot_mt(n: int, threads: int, pool: ThreadPoolExecutor) -> bytes:
    parts = [pool.submit(mandelbrot_rows, n, t, threads) for t in range(threads)]
    rows: list[bytes] = [b""] * n
    for t, job in enumerate(parts):
        rows[t::threads] = job.result()
    return b"".join(rows)


def binary_trees_mt(n: int, threads: int, pool: ThreadPoolExecutor) -> str:
    def depth_run(depth: int, iterations: int) -> int:
        share = iterations // threads
        return sum(job.result() for job in [pool.submit(depth_checks, depth, share) for _ in range(threads)])

    return binary_trees_lines(n, depth_run)
