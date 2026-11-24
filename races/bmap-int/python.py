# Race: bmap-int.go.map (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from collections import Counter, defaultdict
from gen import XorShift32, fnv1a, h31, random_strings


def ints(n: int) -> list[int]:
    return XorShift32().ints(n)


def map_int_input(n: int) -> tuple[list[int], list[int]]:
    inserts = ints(n)
    return inserts, [k ^ (i & 1) for i, k in enumerate(inserts)]


def count_hits(counts: dict, lookups: list) -> str:
    hits = 0
    for k in lookups:
        if k in counts:
            hits += 1
    return f"{len(counts)}:{hits}"


def map_dict(inserts: list, lookups: list) -> str:
    counts = {}
    for k in inserts:
        counts[k] = counts.get(k, 0) + 1
    return count_hits(counts, lookups)


def map_defaultdict(inserts: list, lookups: list) -> str:
    counts: defaultdict = defaultdict(int)
    for k in inserts:
        counts[k] += 1
    return count_hits(counts, lookups)


def map_counter(inserts: list, lookups: list) -> str:
    return count_hits(Counter(inserts), lookups)
