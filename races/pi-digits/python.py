# Race: pi-digits (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

import math
import sys


CHUD_A = 13591409


CHUD_B = 545140134


CHUD_C3_24 = 640320**3 // 24


DIGITS_PER_TERM = 14


GUARD_DIGITS = 10


def binary_split(a: int, b: int) -> tuple[int, int, int]:
    """P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1."""
    if b - a == 1:
        if a == 0:
            p = q = 1
        else:
            p = (6 * a - 5) * (2 * a - 1) * (6 * a - 1)
            q = a * a * a * CHUD_C3_24
        t = p * (CHUD_A + CHUD_B * a)
        return p, q, -t if a % 2 else t
    m = (a + b) // 2
    p1, q1, t1 = binary_split(a, m)
    p2, q2, t2 = binary_split(m, b)
    return p1 * p2, q1 * q2, t1 * q2 + p1 * t2


def isqrt(n: int) -> int:
    """Floor square root of n > 0 by precision doubling (math.isqrt's algorithm, written out as in the others)."""
    c = (n.bit_length() - 1) // 2
    a = 1
    d = 0
    for s in reversed(range(c.bit_length())):
        e = d
        d = c >> s
        a = (a << (d - e - 1)) + (n >> (2 * c - e - d + 1)) // a
    return a - 1 if a * a > n else a


def pi_digits(n: int) -> str:
    terms = n // DIGITS_PER_TERM + 2
    _p, q, t = binary_split(0, terms)
    one = 10 ** (n + GUARD_DIGITS)
    sqrt_c = isqrt(10005 * one * one)
    pi = 426880 * sqrt_c * q // t
    sys.set_int_max_str_digits(0)
    return str(pi // 10**GUARD_DIGITS)


def pi_checksum(digits: str) -> str:
    return f"{len(digits) - 1}:{digits[-50:]}:{sum(map(int, digits))}"
