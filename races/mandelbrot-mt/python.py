# Race: mandelbrot-mt (python)
# Extracted from suite/python/mt.py — build and run via suite/run.py

from concurrent.futures import ThreadPoolExecutor
from bg import binary_trees_lines, depth_checks, mandelbrot_row


def mandelbrot_rows(n: int, first: int, step: int) -> list[bytes]:
    return [mandelbrot_row(n, y) for y in range(first, n, step)]


def mandelbrot_mt(n: int, threads: int, pool: ThreadPoolExecutor) -> bytes:
    parts = [pool.submit(mandelbrot_rows, n, t, threads) for t in range(threads)]
    rows: list[bytes] = [b""] * n
    for t, job in enumerate(parts):
        rows[t::threads] = job.result()
    return b"".join(rows)
