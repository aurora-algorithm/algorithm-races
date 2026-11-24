# Race: bsort-str-reversed.rust.sort (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from gen import XorShift32, fnv1a, h31, random_strings


def strings_reversed(n: int) -> list[str]:
    return sorted(random_strings(n), reverse=True)


def list_sort(a: list) -> list:
    a.sort()
    return a
