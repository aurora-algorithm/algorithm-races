# Race: rain-water (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

import heapq
import math
from gen import MASK, XorShift32, fnv1a, h31


def terrain(side: int) -> list[int]:
    rng = XorShift32()
    return [rng.next() % 1000 for _ in range(side * side)]


def rain_water(heights: list[int]) -> str:
    n = len(heights)
    side = math.isqrt(n)
    seen = bytearray(n)
    heap = []
    for idx in range(n):
        r, c = divmod(idx, side)
        if r in (0, side - 1) or c in (0, side - 1):
            seen[idx] = 1
            heap.append(heights[idx] * n + idx)
    heapq.heapify(heap)
    water = wet = 0
    while heap:
        level, idx = divmod(heapq.heappop(heap), n)
        r, c = divmod(idx, side)
        for nr, nc in ((r - 1, c), (r + 1, c), (r, c - 1), (r, c + 1)):
            if 0 <= nr < side and 0 <= nc < side:
                nidx = nr * side + nc
                if not seen[nidx]:
                    seen[nidx] = 1
                    h = heights[nidx]
                    if h < level:
                        water += level - h
                        wet += 1
                        h = level
                    heapq.heappush(heap, h * n + nidx)
    return f"{side}:{water}:{wet}"
