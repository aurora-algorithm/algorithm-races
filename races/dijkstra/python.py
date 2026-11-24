# Race: dijkstra (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

import heapq
from gen import XorShift32, h31


def graph(n: int) -> tuple[list[int], list[int]]:
    """Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random."""
    rng = XorShift32()
    to = [0] * (4 * n)
    weight = [0] * (4 * n)
    for u in range(n):
        to[4 * u] = (u + 1) % n
        weight[4 * u] = 1 + rng.next() % 100
        for e in range(4 * u + 1, 4 * u + 4):
            to[e] = rng.next() % n
            weight[e] = 1 + rng.next() % 100
    return to, weight


def dijkstra(n: int, to: list[int], weight: list[int]) -> str:
    inf = 1 << 62
    dist = [inf] * n
    dist[0] = 0
    heap = [(0, 0)]
    while heap:
        d, u = heapq.heappop(heap)
        if d > dist[u]:
            continue
        for e in range(4 * u, 4 * u + 4):
            v = to[e]
            nd = d + weight[e]
            if nd < dist[v]:
                dist[v] = nd
                heapq.heappush(heap, (nd, v))
    reached = [d for d in dist if d < inf]
    return f"{len(reached)}:{sum(reached)}:{max(reached)}"
