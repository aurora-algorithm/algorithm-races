# Race: knapsack (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from gen import XorShift32, h31


KNAPSACK_ITEMS = 1000


def items() -> list[tuple[int, int]]:
    rng = XorShift32()
    out = []
    for _ in range(KNAPSACK_ITEMS):
        weight = 1 + rng.next() % 1000
        out.append((weight, 1 + rng.next() % 1000))
    return out


def knapsack(capacity: int, things: list[tuple[int, int]]) -> str:
    best = [0] * (capacity + 1)
    for weight, value in things:
        for c in range(capacity, weight - 1, -1):
            if best[c - weight] + value > best[c]:
                best[c] = best[c - weight] + value
    return f"{best[capacity]}:{h31(best)}"
