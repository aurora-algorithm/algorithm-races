# Race: bsort-int.go.zermelo.Sort (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from gen import XorShift32, fnv1a, h31, random_strings


def ints(n: int) -> list[int]:
    return XorShift32().ints(n)
