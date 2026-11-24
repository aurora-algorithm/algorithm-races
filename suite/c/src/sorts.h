#ifndef KERNELS_SORTS_H
#define KERNELS_SORTS_H

#include "common.h"

/* Top-down mergesort of the half-open range [lo, hi) of a, with aux as scratch space. */
void merge_sort(int64_t *a, int64_t *aux, size_t lo, size_t hi);
/* Merges the sorted runs [lo, mid) and [mid, hi) of a; stable. */
void merge(int64_t *a, int64_t *aux, size_t lo, size_t mid, size_t hi);

/* qsort comparators: int64_t by value ((a > b) - (a < b), no overflow); char * by strcmp. */
int compare_i64(const void *pa, const void *pb);
int compare_str(const void *pa, const void *pb);

/* strings(N): len = 8 + next() % 9, then len letters 'a' + next() % 26; all in one block (*block),
 * NUL after each. */
char **strings(size_t n, char **block);

typedef void (*SortFn)(int64_t *a, size_t n, size_t threads);

/* Sorts a fresh copy of ints(N) per repeat with `sort`; checksum h31 of the sorted array. */
bool run_sort(const Args *args, Result *out, SortFn sort);

#endif
