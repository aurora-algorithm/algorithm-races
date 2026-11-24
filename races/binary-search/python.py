# Race: binary-search (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from gen import XorShift32, h31


def sorted_array(n: int) -> tuple[list[int], list[int]]:
    rng = XorShift32()
    a = [0] * n
    v = 0
    for i in range(n):
        v += 1 + rng.next() % 4
        a[i] = v
    return a, [rng.next() % (v + 1) for _ in range(n)]


def binary_search(a: list[int], queries: list[int]) -> str:
    n = len(a)
    hits = total = 0
    for q in queries:
        lo, hi = 0, n
        while lo < hi:
            mid = (lo + hi) // 2
            if a[mid] < q:
                lo = mid + 1
            else:
                hi = mid
        if lo < n and a[lo] == q:
            hits += 1
        total += lo
    return f"{hits}:{total}"
