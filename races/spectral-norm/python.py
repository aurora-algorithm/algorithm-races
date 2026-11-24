# Race: spectral-norm (python)
# Extracted from suite/python/bg.py — build and run via suite/run.py

import math


def eval_a(i: int, j: int) -> float:
    return 1.0 / ((i + j) * (i + j + 1) // 2 + i + 1)


def mul_av(u: list[float], n: int) -> list[float]:
    out = [0.0] * n
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += eval_a(i, j) * u[j]
        out[i] = s
    return out


def mul_atv(u: list[float], n: int) -> list[float]:
    out = [0.0] * n
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += eval_a(j, i) * u[j]
        out[i] = s
    return out


def mul_atav(u: list[float], n: int) -> list[float]:
    return mul_atv(mul_av(u, n), n)


def spectral_norm(n: int) -> str:
    u = [1.0] * n
    v = u
    for _ in range(10):
        v = mul_atav(u, n)
        u = mul_atav(v, n)
    vbv = 0.0
    vv = 0.0
    for i in range(n):
        vbv += u[i] * v[i]
        vv += v[i] * v[i]
    return f"{math.sqrt(vbv / vv):.9f}"
