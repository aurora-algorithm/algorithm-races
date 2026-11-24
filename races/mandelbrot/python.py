# Race: mandelbrot (python)
# Extracted from suite/python/bg.py — build and run via suite/run.py

def mandelbrot_row(n: int, y: int) -> bytes:
    row = bytearray((n + 7) // 8)
    ci = 2.0 * y / n - 1.0
    for x in range(n):
        cr = 2.0 * x / n - 1.5
        zr = zi = tr = ti = 0.0
        i = 0
        while i < 50 and tr + ti <= 4.0:
            zi = 2.0 * zr * zi + ci
            zr = tr - ti + cr
            tr = zr * zr
            ti = zi * zi
            i += 1
        if tr + ti <= 4.0:
            row[x >> 3] |= 0x80 >> (x & 7)
    return bytes(row)


def mandelbrot(n: int) -> bytes:
    return b"".join(mandelbrot_row(n, y) for y in range(n))
