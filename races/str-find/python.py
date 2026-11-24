# Race: str-find (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

import time
from collections.abc import Iterator
from gen import XorShift32, h31


ACGT = "acgt"


PATTERN_LEN = 8


CHUNK = 1 << 20


def chunks(rng: XorShift32, n: int) -> Iterator[list[int]]:
    """The next n generator values, a million at a time (setup speed only; the values are the same)."""
    for start in range(0, n, CHUNK):
        yield rng.ints(min(CHUNK, n - start))


def dna(length: int, count: int) -> list[str]:
    rng = XorShift32()
    return ["".join("".join([ACGT[v % 4] for v in part]) for part in chunks(rng, length)) for _ in range(count)]


def text(n: int) -> tuple[str, str]:
    t = dna(n, 1)[0]
    return t, t[n // 2 : n // 2 + PATTERN_LEN]


def str_find(t: str, pattern: str) -> tuple[int, int, int]:
    count, first, last = 0, -1, -1
    pos = t.find(pattern)
    while pos >= 0:
        if first < 0:
            first = pos
        last = pos
        count += 1
        pos = t.find(pattern, pos + 1)
    return count, first, last


def matches(result: tuple[int, int, int]) -> str:
    return "{}:{}:{}".format(*result)
