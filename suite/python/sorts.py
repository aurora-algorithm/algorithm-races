"""Sorting kernels, written the same way in every language (kernels/SPEC.md section 3)."""


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


def run_quicksort(a: list[int]) -> list[int]:
    quicksort(a, 0, len(a) - 1)
    return a


def run_mergesort(a: list[int]) -> list[int]:
    merge_sort(a, [0] * len(a), 0, len(a))
    return a


def run_heapsort(a: list[int]) -> list[int]:
    heapsort(a)
    return a


def run_builtin(a: list[int]) -> list[int]:
    a.sort()
    return a
