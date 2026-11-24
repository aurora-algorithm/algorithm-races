# Race: bwt (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


DNA = b"ACGT"


def dna_text(n: int) -> bytes:
    """`ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value."""
    rng = XorShift32()
    out = bytearray(n)
    for i in range(0, n, 16):
        v = rng.next()
        for j in range(i, min(i + 16, n)):
            out[j] = DNA[v & 3]
            v >>= 2
    return bytes(out)


def bwt(dna: bytes) -> tuple[int, int, int, bytes]:
    text = dna + b"$"
    n = len(text)
    rank = list(text)
    sa = list(range(n))
    k = 1
    rounds = 0
    while True:
        rounds += 1
        second = [rank[i + k] if i + k < n else -1 for i in range(n)]
        sa.sort(key=lambda i: (rank[i], second[i]))
        tmp = [0] * n
        for j in range(1, n):
            a, b = sa[j - 1], sa[j]
            tmp[b] = tmp[a] + (rank[a] != rank[b] or second[a] != second[b])
        rank = tmp
        if rank[sa[-1]] == n - 1:
            break
        k *= 2
    out = bytearray(n)
    primary = 0
    for j, i in enumerate(sa):
        if i == 0:
            primary = j
            out[j] = text[-1]
        else:
            out[j] = text[i - 1]
    return n, rounds, primary, bytes(out)


def bwt_checksum(result: tuple[int, int, int, bytes]) -> str:
    n, rounds, primary, out = result
    return f"{n}:{rounds}:{primary}:{fnv1a(out)}"
