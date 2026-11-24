# Race: heapsort (python)
# Extracted from suite/python/sorts.py — build and run via suite/run.py

def sift_down(a: list[int], root: int, end: int) -> None:
    while 2 * root + 1 < end:
        child = 2 * root + 1
        if child + 1 < end and a[child] < a[child + 1]:
            child += 1
        if a[root] < a[child]:
            a[root], a[child] = a[child], a[root]
            root = child
        else:
            return


def heapsort(a: list[int]) -> None:
    n = len(a)
    for start in range(n // 2 - 1, -1, -1):
        sift_down(a, start, n)
    for end in range(n - 1, 0, -1):
        a[0], a[end] = a[end], a[0]
        sift_down(a, 0, end)


def run_heapsort(a: list[int]) -> list[int]:
    heapsort(a)
    return a
