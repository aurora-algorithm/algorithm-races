# Race: kruskal (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from operator import itemgetter


def find(parent: list[int], x: int) -> int:
    root = x
    while parent[root] != root:
        root = parent[root]
    while parent[x] != root:
        parent[x], x = root, parent[x]
    return root


def kruskal(n: int, edges: list[tuple[int, int, int]]) -> str:
    edges.sort(key=itemgetter(0))
    parent = list(range(n))
    rank = [0] * n
    count = total = 0
    for w, u, v in edges:
        ru = find(parent, u)
        rv = find(parent, v)
        if ru == rv:
            continue
        if rank[ru] < rank[rv]:
            ru, rv = rv, ru
        parent[rv] = ru
        if rank[ru] == rank[rv]:
            rank[ru] += 1
        count += 1
        total += w
    return f"{count}:{total}"
