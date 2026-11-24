"""The builtins group (kernels/builtins-NOTES.md): the standard library's ways of doing one job, one way per kernel.

Kernel names are JOB.python.WAY (kernels/builtins/ways.json). Every way of a job gets the same input and must
print the same checksum; only the call that does the job differs.
"""

import io
from collections import Counter, defaultdict
from collections.abc import Callable

from gen import XorShift32, fnv1a, h31, random_strings

LANG = "python"


def ints(n: int) -> list[int]:
    return XorShift32().ints(n)


def ints_sorted(n: int) -> list[int]:
    return sorted(ints(n))


def ints_reversed(n: int) -> list[int]:
    return sorted(ints(n), reverse=True)


def strings_sorted(n: int) -> list[str]:
    return sorted(random_strings(n))


def strings_reversed(n: int) -> list[str]:
    return sorted(random_strings(n), reverse=True)


INT_INPUTS: dict[str, Callable[[int], list[int]]] = {
    "bsort-int": ints,
    "bsort-int-sorted": ints_sorted,
    "bsort-int-reversed": ints_reversed,
}
STR_INPUTS: dict[str, Callable[[int], list[str]]] = {
    "bsort-str": random_strings,
    "bsort-str-sorted": strings_sorted,
    "bsort-str-reversed": strings_reversed,
}


def ints_checksum(a) -> str:
    return str(h31(a))


def strings_checksum(a) -> str:
    return str(fnv1a("".join(s + "\n" for s in a).encode()))


def map_int_input(n: int) -> tuple[list[int], list[int]]:
    inserts = ints(n)
    return inserts, [k ^ (i & 1) for i, k in enumerate(inserts)]


def map_str_input(n: int) -> tuple[list[str], list[str]]:
    """Even i looks up a key that is in the map; odd i the key without its last letter (almost never in it)."""
    inserts = random_strings(n)
    return inserts, [k if i % 2 == 0 else k[:-1] for i, k in enumerate(inserts)]


def fresh_strings(kv: tuple[list[str], list[str]]) -> tuple[list[str], list[str]]:
    """New str objects with the same text for every repeat, for the inserts and separately for the lookups (made
    untimed): a str caches its hash, and an equal key that is the same object matches by identity."""
    inserts, lookups = kv
    return [k.encode().decode() for k in inserts], [k.encode().decode() for k in lookups]


def join_parts(n: int) -> list[str]:
    return [str(v) for v in ints(n)]


def joined_checksum(s: str) -> str:
    data = s.encode()
    return f"{len(data)}:{fnv1a(data)}"


def list_sort(a: list) -> list:
    a.sort()
    return a


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


def join_str_join(parts: list[str]) -> str:
    return ",".join(parts)


def join_plus_equals(parts: list[str]) -> str:
    out = ""
    for i, part in enumerate(parts):
        if i:
            out += ","
        out += part
    return out


def join_string_io(parts: list[str]) -> str:
    buf = io.StringIO()
    for i, part in enumerate(parts):
        if i:
            buf.write(",")
        buf.write(part)
    return buf.getvalue()


SORTS = {"list.sort": list_sort, "sorted": sorted}
MAPS = {"dict": map_dict, "defaultdict": map_defaultdict, "Counter": map_counter}
JOINS = {"str.join": join_str_join, "+=": join_plus_equals, "io.StringIO": join_string_io}


def entry(prepare, run, check, fresh=None) -> dict:
    k = {"prepare": lambda n, _d: prepare(n), "run": lambda x, _t, _p: run(x), "check": check}
    if fresh:
        k["fresh"] = fresh
    return k


def kernels() -> dict:
    table = {}
    for job, make in INT_INPUTS.items():
        for way, fn in SORTS.items():
            table[f"{job}.{LANG}.{way}"] = entry(make, fn, ints_checksum, list.copy)
    for job, make in STR_INPUTS.items():
        for way, fn in SORTS.items():
            table[f"{job}.{LANG}.{way}"] = entry(make, fn, strings_checksum, list.copy)
    for way, fn in MAPS.items():
        table[f"bmap-int.{LANG}.{way}"] = entry(map_int_input, lambda kv, f=fn: f(*kv), str)
        table[f"bmap-str.{LANG}.{way}"] = entry(map_str_input, lambda kv, f=fn: f(*kv), str, fresh_strings)
    for way, fn in JOINS.items():
        table[f"bjoin.{LANG}.{way}"] = entry(join_parts, fn, joined_checksum)
    return table
