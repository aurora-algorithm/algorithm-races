# Race: matmul (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

def matmul(a: list[float], b: list[float], n: int) -> list[float]:
    c = [0.0] * (n * n)
    for i in range(n):
        row = i * n
        for j in range(n):
            s = 0.0
            for k in range(n):
                s += a[row + k] * b[k * n + j]
            c[row + j] = s
    return c
