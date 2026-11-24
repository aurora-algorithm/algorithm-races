# Race: bfs-grid (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

import time
from collections import OrderedDict, deque
from collections.abc import Iterator
from gen import XorShift32, h31


CHUNK = 1 << 20


def chunks(rng: XorShift32, n: int) -> Iterator[list[int]]:
    """The next n generator values, a million at a time (setup speed only; the values are the same)."""
    for start in range(0, n, CHUNK):
        yield rng.ints(min(CHUNK, n - start))


def grid(n: int) -> bytearray:
    walls = bytearray()
    for part in chunks(XorShift32(), n * n):
        walls += bytes(1 if v % 100 < 30 else 0 for v in part)
    for cell in (0, 1, n, n * n - n - 1, n * n - 2, n * n - 1):
        walls[cell] = 0
    return walls


def bfs_grid(n: int, walls: bytearray) -> str:
    dist = [-1] * (n * n)
    dist[0] = 0
    queue = deque([0])
    while queue:
        cell = queue.popleft()
        d = dist[cell] + 1
        row, col = divmod(cell, n)
        for nxt, ok in ((cell - n, row > 0), (cell + n, row < n - 1), (cell - 1, col > 0), (cell + 1, col < n - 1)):
            if ok and not walls[nxt] and dist[nxt] < 0:
                dist[nxt] = d
                queue.append(nxt)
    reached = [d for d in dist if d >= 0]
    return f"{len(reached)}:{dist[-1]}:{sum(reached)}"
