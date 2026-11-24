# Race: hashmap (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

def hashmap(inserts: list[int], lookups: list[int]) -> str:
    counts: dict[int, int] = {}
    for k in inserts:
        counts[k] = counts.get(k, 0) + 1
    hits = 0
    for k in lookups:
        if k in counts:
            hits += 1
    return f"{len(counts)}:{hits}"
