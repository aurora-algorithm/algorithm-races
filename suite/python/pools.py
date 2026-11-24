"""The multi-threaded kernels on a pool of subinterpreters or processes instead of threads (kernels/SPEC.md section 14).

usage: python pools.py interpreters|processes KERNEL --size N [--repeats R] [--threads T] ...
main.py runs the same kernels on threads (py314: one GIL, py314t: free-threaded). Here each worker has its own
interpreter: `concurrent.futures.InterpreterPoolExecutor` (one interpreter with its own GIL per worker thread, new in
3.14) or `ProcessPoolExecutor` (one process per worker). Arguments and results are pickled between them, so nothing
is shared: mergesort-mt sends each chunk to a worker and gets it back sorted (mt_copy.py), the other two kernels
already return their results. The workers start before the timed region, like the thread pool in main.py.
"""

import sys
import time
from concurrent.futures import Executor, InterpreterPoolExecutor, ProcessPoolExecutor

import main as driver
import mt
import mt_copy

POOLS: dict[str, type[Executor]] = {"interpreters": InterpreterPoolExecutor, "processes": ProcessPoolExecutor}
MT_KERNELS = ("mergesort-mt", "mandelbrot-mt", "binary-trees-mt")
WARMUP_S = 0.05


def started_pool(kind: str, workers: int) -> Executor:
    """Both pools start a worker per submitted task while none is idle: hold T tasks at once so all T exist."""
    pool = POOLS[kind](max_workers=workers)
    for job in [pool.submit(time.sleep, WARMUP_S) for _ in range(workers)]:
        job.result()
    return pool


def main() -> int:
    kind = sys.argv.pop(1) if len(sys.argv) > 1 and sys.argv[1] in POOLS else ""
    if not kind or len(sys.argv) < 2 or sys.argv[1] not in MT_KERNELS:
        print(f"usage: pools.py {'|'.join(POOLS)} {'|'.join(MT_KERNELS)} --size N ...", file=sys.stderr)
        return 2
    mt.mergesort_mt = mt_copy.mergesort_mt
    driver.start_pool = lambda workers: started_pool(kind, workers)
    plain = driver.runtime_info
    driver.runtime_info = lambda use_numpy, kernel: f"{plain(use_numpy, kernel)} pool={POOLS[kind].__name__}"
    return driver.main()


if __name__ == "__main__":
    sys.exit(main())
