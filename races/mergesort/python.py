# Race: mergesort (python)
# Extracted from suite/python/sorts.py — build and run via suite/run.py

def merge_sort(a: list[int], aux: list[int], lo: int, hi: int) -> None:
    if hi - lo < 2:
        return
    mid = lo + (hi - lo) // 2
    merge_sort(a, aux, lo, mid)
    merge_sort(a, aux, mid, hi)
    merge(a, aux, lo, mid, hi)


def merge(a: list[int], aux: list[int], lo: int, mid: int, hi: int) -> None:
    aux[lo:hi] = a[lo:hi]
    i, j, k = lo, mid, lo
    while i < mid and j < hi:
        if aux[j] < aux[i]:
            a[k] = aux[j]
            j += 1
        else:
            a[k] = aux[i]
            i += 1
        k += 1
    while i < mid:
        a[k] = aux[i]
        i += 1
        k += 1
    while j < hi:
        a[k] = aux[j]
        j += 1
        k += 1


def run_mergesort(a: list[int]) -> list[int]:
    merge_sort(a, [0] * len(a), 0, len(a))
    return a
