# Race: lis (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from bisect import bisect_left
from gen import XorShift32, h31


def lis(values: list[int]) -> str:
    tails: list[int] = []
    for x in values:
        i = bisect_left(tails, x)
        if i == len(tails):
            tails.append(x)
        else:
            tails[i] = x
    return f"{len(tails)}:{h31(tails)}"
