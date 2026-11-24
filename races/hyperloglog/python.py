# Race: hyperloglog (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

import math
from gen import MASK, XorShift32, fnv1a, h31


def fmix32(h: int) -> int:
    """MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels."""
    h ^= h >> 16
    h = (h * 0x85EBCA6B) & MASK
    h ^= h >> 13
    h = (h * 0xC2B2AE35) & MASK
    h ^= h >> 16
    return h


HLL_P = 14


HLL_M = 1 << HLL_P


def hyperloglog(n: int) -> str:
    rng = XorShift32()
    universe = max(n // 4, 1)
    registers = bytearray(HLL_M)
    for _ in range(n):
        h = fmix32(rng.next() % universe)
        w = (h << HLL_P) & MASK
        rank = 33 - w.bit_length() if w else 32 - HLL_P + 1
        idx = h >> (32 - HLL_P)
        if rank > registers[idx]:
            registers[idx] = rank
    z = 0.0
    zeros = 0
    for r in registers:
        z += 2.0**-r
        zeros += r == 0
    m = float(HLL_M)
    estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z
    if estimate <= 2.5 * m and zeros:
        estimate = m * math.log(m / zeros)
    return f"{n}:{math.floor(estimate + 0.5)}:{h31(registers)}"
