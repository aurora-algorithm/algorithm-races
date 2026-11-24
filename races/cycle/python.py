# Race: cycle (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


CYCLE_MOD = 67108859


CYCLE_C = 12345


def cycle_starts(n: int) -> list[int]:
    rng = XorShift32()
    return [rng.next() % CYCLE_MOD for _ in range(n)]


def cycle(starts: list[int]) -> str:
    m, c = CYCLE_MOD, CYCLE_C
    total_mu = total_lam = longest = 0
    for x0 in starts:
        tortoise = (x0 * x0 + c) % m
        hare = (tortoise * tortoise + c) % m
        while tortoise != hare:
            tortoise = (tortoise * tortoise + c) % m
            hare = (hare * hare + c) % m
            hare = (hare * hare + c) % m
        mu = 0
        tortoise = x0
        while tortoise != hare:
            tortoise = (tortoise * tortoise + c) % m
            hare = (hare * hare + c) % m
            mu += 1
        lam = 1
        hare = (tortoise * tortoise + c) % m
        while tortoise != hare:
            hare = (hare * hare + c) % m
            lam += 1
        total_mu += mu
        total_lam += lam
        longest = max(longest, mu + lam)
    return f"{len(starts)}:{total_mu}:{total_lam}:{longest}"
