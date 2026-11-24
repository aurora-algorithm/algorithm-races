# Race: edit-distance (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

def edit_distance(a: str, b: str) -> str:
    prev = list(range(len(b) + 1))
    cur = [0] * (len(b) + 1)
    for i, ca in enumerate(a, 1):
        cur[0] = i
        for j, cb in enumerate(b, 1):
            cost = 0 if ca == cb else 1
            cur[j] = min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost)
        prev, cur = cur, prev
    return str(prev[len(b)])
