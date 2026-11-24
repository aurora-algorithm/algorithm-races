# Race: lru (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from collections import OrderedDict, deque
from gen import XorShift32, h31


def lru_keys(n: int) -> tuple[int, list[int]]:
    capacity = max(1, n // 100)
    return capacity, [k % (2 * capacity) for k in XorShift32().ints(n)]


def lru(capacity: int, keys: list[int]) -> str:
    cache: OrderedDict[int, int] = OrderedDict()
    hits = 0
    for i, k in enumerate(keys):
        if k in cache:
            cache.move_to_end(k)
            hits += 1
        else:
            if len(cache) == capacity:
                cache.popitem(last=False)
            cache[k] = i
    return f"{hits}:{len(keys) - hits}:{h31(cache)}"
