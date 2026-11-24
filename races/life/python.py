# Race: life (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


LIFE_SIDE = 512


def life_input(gens: int) -> tuple[bytes, int]:
    rng = XorShift32()
    return bytes(1 if rng.next() % 4 == 0 else 0 for _ in range(LIFE_SIDE * LIFE_SIDE)), gens


def life(grid: bytes, gens: int) -> tuple[int, bytearray]:
    w = LIFE_SIDE
    cur = bytearray(grid)
    nxt = bytearray(len(grid))
    for _ in range(gens):
        for y in range(w):
            up = (y + w - 1) % w * w
            row = y * w
            down = (y + 1) % w * w
            for x in range(w):
                left = (x + w - 1) % w
                right = (x + 1) % w
                n = (
                    cur[up + left]
                    + cur[up + x]
                    + cur[up + right]
                    + cur[row + left]
                    + cur[row + right]
                    + cur[down + left]
                    + cur[down + x]
                    + cur[down + right]
                )
                nxt[row + x] = 1 if n == 3 or (n == 2 and cur[row + x]) else 0
        cur, nxt = nxt, cur
    return gens, cur


def life_checksum(result: tuple[int, bytearray]) -> str:
    gens, grid = result
    return f"{gens}:{sum(grid)}:{fnv1a(grid)}"
