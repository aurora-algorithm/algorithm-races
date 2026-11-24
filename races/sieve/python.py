# Race: sieve (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

def sieve(n: int) -> str:
    composite = bytearray(n + 1)
    i = 2
    while i * i <= n:
        if not composite[i]:
            for j in range(i * i, n + 1, i):
                composite[j] = 1
        i += 1
    count = 0
    for i in range(2, n + 1):
        if not composite[i]:
            count += 1
    return str(count)
