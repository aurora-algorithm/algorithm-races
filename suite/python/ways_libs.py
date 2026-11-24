"""The builtins group's library rows for Python: NumPy and Polars sorts (kernels/builtins-NOTES.md).

Run by the `py-libs` runtime (CPython 3.14 in kernels/.venv-builtins, python/requirements-builtins.txt). The
libraries are imported while the input is built, so a plain CPython without them never imports anything from here.
Kernel names are JOB.python.WAY; the inputs and checksums are those of ways.py.
"""

import os

import ways

KINDS = ("quicksort", "stable", "heapsort")


def libraries() -> str:
    from importlib.metadata import version

    return f"numpy {version('numpy')}, polars {version('polars')}, pyahocorasick {version('pyahocorasick')}"


def numpy_ints(make):
    def prepare(n: int):
        import numpy as np

        return np.array(make(n), dtype=np.int64)

    return prepare


def numpy_strings(make):
    def prepare(n: int):
        import numpy as np

        return np.array(make(n))

    return prepare


def polars_series(make):
    def prepare(n: int):
        import polars as pl

        return pl.Series(make(n))

    return prepare


def ndarray_sort(kind: str):
    def run(a):
        a.sort(kind=kind)
        return a

    return run


def series_sort(s):
    return s.sort()


def as_list(check):
    return lambda a: check(a.tolist())


def with_pool_size(check):
    """Polars sorts on its thread pool (POLARS_MAX_THREADS = --threads); `extra` records the size it used."""

    def checksum(s):
        import polars as pl

        return check(s.to_list()), {"pool_threads": pl.thread_pool_size()}

    return checksum


def numpy_entry(make, to_numpy, kind: str, check) -> dict:
    prepare, run = to_numpy(make), ndarray_sort(kind)
    return {
        "prepare": lambda n, _d: prepare(n),
        "fresh": lambda a: a.copy(),
        "run": lambda a, _t, _p: run(a),
        "check": as_list(check),
    }


def polars_entry(make, check) -> dict:
    prepare = polars_series(make)
    return {
        "prepare": lambda n, _d: prepare(n),
        "run": lambda s, _t, _p: series_sort(s),
        "check": with_pool_size(check),
    }


def kernels(threads: int) -> dict:
    os.environ.setdefault("POLARS_MAX_THREADS", str(threads))
    table = {}
    groups = [
        (ways.INT_INPUTS, numpy_ints, ways.ints_checksum),
        (ways.STR_INPUTS, numpy_strings, ways.strings_checksum),
    ]
    for inputs, to_numpy, check in groups:
        for job, make in inputs.items():
            for kind in KINDS:
                table[f"{job}.{ways.LANG}.ndarray.sort-{kind}"] = numpy_entry(make, to_numpy, kind, check)
            table[f"{job}.{ways.LANG}.polars.Series.sort"] = polars_entry(make, check)
    return table
