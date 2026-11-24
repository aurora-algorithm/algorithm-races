# Race: bloom (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


def fmix32(h: int) -> int:
    """MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels."""
    h ^= h >> 16
    h = (h * 0x85EBCA6B) & MASK
    h ^= h >> 13
    h = (h * 0xC2B2AE35) & MASK
    h ^= h >> 16
    return h


BLOOM_BITS_PER_KEY = 10


BLOOM_HASHES = 7


GOLDEN = 0x9E3779B9


def bloom_input(n: int) -> tuple[list[int], list[int]]:
    values = XorShift32().ints(2 * n)
    return values[:n], values[n:]


def bloom(keys: list[int], lookups: list[int]) -> str:
    m = BLOOM_BITS_PER_KEY * len(keys)
    bits = bytearray((m + 7) // 8)
    for key in keys:
        h1 = fmix32(key)
        h2 = fmix32(key ^ GOLDEN) | 1
        for i in range(BLOOM_HASHES):
            b = ((h1 + i * h2) & MASK) % m
            bits[b >> 3] |= 1 << (b & 7)
    false_positives = 0
    for key in lookups:
        h1 = fmix32(key)
        h2 = fmix32(key ^ GOLDEN) | 1
        for i in range(BLOOM_HASHES):
            b = ((h1 + i * h2) & MASK) % m
            if not bits[b >> 3] & (1 << (b & 7)):
                break
        else:
            false_positives += 1
    set_bits = sum(byte.bit_count() for byte in bits)
    return f"{len(keys)}:{set_bits}:{false_positives}"
