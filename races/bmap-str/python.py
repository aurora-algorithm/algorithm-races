# Race: bmap-str.go.map (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from collections import Counter, defaultdict
from gen import XorShift32, fnv1a, h31, random_strings


def map_str_input(n: int) -> tuple[list[str], list[str]]:
    """Even i looks up a key that is in the map; odd i the key without its last letter (almost never in it)."""
    inserts = random_strings(n)
    return inserts, [k if i % 2 == 0 else k[:-1] for i, k in enumerate(inserts)]


def fresh_strings(kv: tuple[list[str], list[str]]) -> tuple[list[str], list[str]]:
    """New str objects with the same text for every repeat, for the inserts and separately for the lookups (made
    untimed): a str caches its hash, and an equal key that is the same object matches by identity."""
    inserts, lookups = kv
    return [k.encode().decode() for k in inserts], [k.encode().decode() for k in lookups]


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
