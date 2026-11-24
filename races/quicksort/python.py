# Race: quicksort (python)
# Extracted from suite/python/sorts.py — build and run via suite/run.py

def quicksort(a: list[int], lo: int, hi: int) -> None:
    while lo < hi:
        pivot = a[lo + (hi - lo) // 2]
        i = lo - 1
        j = hi + 1
        while True:
            i += 1
            while a[i] < pivot:
                i += 1
            j -= 1
            while a[j] > pivot:
                j -= 1
            if i >= j:
                break
            a[i], a[j] = a[j], a[i]
        if j - lo < hi - j:
            quicksort(a, lo, j)
            lo = j + 1
        else:
            quicksort(a, j + 1, hi)
            hi = j


def run_quicksort(a: list[int]) -> list[int]:
    quicksort(a, 0, len(a) - 1)
    return a
