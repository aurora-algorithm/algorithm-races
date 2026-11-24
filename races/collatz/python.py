# Race: collatz (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

def collatz(n: int) -> str:
    cache = [0] * n
    cache[1] = 1
    best, best_len, total = 1, 1, 1
    for start in range(2, n):
        x = start
        steps = 0
        while x >= start:
            x = x // 2 if x % 2 == 0 else 3 * x + 1
            steps += 1
        length = steps + cache[x]
        cache[start] = length
        total += length
        if length > best_len:
            best, best_len = start, length
    return f"{n}:{best}:{best_len}:{total}"
