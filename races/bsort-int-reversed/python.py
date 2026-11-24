# Race: bsort-int-reversed.goose.sort (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from gen import XorShift32, fnv1a, h31, random_strings


def ints(n: int) -> list[int]:
    return XorShift32().ints(n)


def ints_reversed(n: int) -> list[int]:
    return sorted(ints(n), reverse=True)


def list_sort(a: list) -> list:
    a.sort()
    return a
