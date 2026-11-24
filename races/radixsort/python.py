# Race: radixsort (python)
# Extracted from suite/python/sorts.py — build and run via suite/run.py

def radix_sort(a: list[int]) -> list[int]:
    buf = [0] * len(a)
    for shift in (0, 8, 16, 24):
        count = [0] * 256
        for v in a:
            count[(v >> shift) & 255] += 1
        pos = [0] * 256
        total = 0
        for d in range(256):
            pos[d] = total
            total += count[d]
        for v in a:
            d = (v >> shift) & 255
            buf[pos[d]] = v
            pos[d] += 1
        a, buf = buf, a
    return a
