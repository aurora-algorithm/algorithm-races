# Race: palindrome (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

def palindrome(text: bytes) -> str:
    t = bytearray(b"#") * (2 * len(text) + 3)
    t[0], t[-1] = ord("^"), ord("$")
    t[2:-2:2] = text
    p = [0] * len(t)
    center = right = 0
    for i in range(1, len(t) - 1):
        if i < right:
            p[i] = min(right - i, p[2 * center - i])
        while t[i + 1 + p[i]] == t[i - 1 - p[i]]:
            p[i] += 1
        if i + p[i] > right:
            center, right = i, i + p[i]
    longest = best = count = 0
    for i, radius in enumerate(p):
        count += (radius + 1) // 2
        if radius > longest:
            longest, best = radius, i
    return f"{len(text)}:{longest}:{(best - 1 - longest) // 2}:{count}"
