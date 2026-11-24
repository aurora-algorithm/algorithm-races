# Race: binary-trees-mt (python)
# Extracted from suite/python/mt.py — build and run via suite/run.py

from concurrent.futures import ThreadPoolExecutor
from bg import binary_trees_lines, depth_checks, mandelbrot_row


def binary_trees_mt(n: int, threads: int, pool: ThreadPoolExecutor) -> str:
    def depth_run(depth: int, iterations: int) -> int:
        share = iterations // threads
        return sum(job.result() for job in [pool.submit(depth_checks, depth, share) for _ in range(threads)])

    return binary_trees_lines(n, depth_run)
