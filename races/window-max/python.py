# Race: window-max (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


WINDOWS = (10, 100, 1000, 10000, 100000)


def window_max(values: list[int]) -> str:
    n = len(values)
    dq = [0] * n
    hashes = []
    for w in WINDOWS:
        head = tail = 0
        h = 0
        for i in range(n):
            v = values[i]
            while tail > head and values[dq[tail - 1]] <= v:
                tail -= 1
            dq[tail] = i
            tail += 1
            if dq[head] <= i - w:
                head += 1
            if i >= w - 1:
                h = (h * 31 + values[dq[head]]) & MASK
        hashes.append(str(h))
    return f"{n}:" + ":".join(hashes)
