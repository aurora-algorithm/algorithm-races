/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

int compare_i64(const void *pa, const void *pb) {
    int64_t a = *(const int64_t *)pa;
    int64_t b = *(const int64_t *)pb;
    return (a > b) - (a < b);
}

static void std_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    qsort(a, n, sizeof *a, compare_i64);
}

static void swap(int64_t *a, size_t i, size_t j) {
    int64_t t = a[i];
    a[i] = a[j];
    a[j] = t;
}

/* Hoare partition with the middle element as pivot; inclusive bounds.
 * Recurses into the smaller side and loops on the larger one. */
static void quicksort_range(int64_t *a, size_t lo, size_t hi) {
    while (lo < hi) {
        int64_t pivot = a[lo + (hi - lo) / 2];
        size_t i = lo;
        size_t j = hi;
        for (;;) {
            while (a[i] < pivot) {
                i++;
            }
            while (a[j] > pivot) {
                j--;
            }
            if (i >= j) {
                break;
            }
            swap(a, i, j);
            i++;
            j--;
        }
        if (j - lo < hi - j) {
            quicksort_range(a, lo, j);
            lo = j + 1;
        } else {
            quicksort_range(a, j + 1, hi);
            hi = j;
        }
    }
}

static void quick_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    if (n > 1) {
        quicksort_range(a, 0, n - 1);
    }
}

void merge_sort(int64_t *a, int64_t *aux, size_t lo, size_t hi) {
    if (hi - lo < 2) {
        return;
    }
    size_t mid = lo + (hi - lo) / 2;
    merge_sort(a, aux, lo, mid);
    merge_sort(a, aux, mid, hi);
    merge(a, aux, lo, mid, hi);
}

void merge(int64_t *a, int64_t *aux, size_t lo, size_t mid, size_t hi) {
    memcpy(aux + lo, a + lo, (hi - lo) * sizeof *a);
    size_t i = lo;
    size_t j = mid;
    size_t k = lo;
    while (i < mid && j < hi) {
        if (aux[j] < aux[i]) {
            a[k++] = aux[j++];
        } else {
            a[k++] = aux[i++];
        }
    }
    while (i < mid) {
        a[k++] = aux[i++];
    }
    while (j < hi) {
        a[k++] = aux[j++];
    }
}

static void merge_sort_all(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    int64_t *aux = xmalloc(n * sizeof *aux);
    merge_sort(a, aux, 0, n);
    free(aux);
}

static void sift_down(int64_t *a, size_t root, size_t end) {
    while (2 * root + 1 < end) {
        size_t child = 2 * root + 1;
        if (child + 1 < end && a[child] < a[child + 1]) {
            child++;
        }
        if (a[root] < a[child]) {
            swap(a, root, child);
            root = child;
        } else {
            return;
        }
    }
}

static void heap_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    for (size_t start = n / 2; start-- > 0;) {
        sift_down(a, start, n);
    }
    for (size_t end = n; end-- > 1;) {
        swap(a, 0, end);
        sift_down(a, 0, end);
    }
}

/* LSD radix sort, 8 bits per pass, for values below 2^32; after 4 passes the result is back in a.
 */
static void radix_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    int64_t *buf = xmalloc(n * sizeof *buf);
    int64_t *from = a;
    int64_t *to = buf;
    for (int shift = 0; shift < 32; shift += 8) {
        size_t count[256] = {0};
        for (size_t i = 0; i < n; i++) {
            count[(from[i] >> shift) & 255]++;
        }
        size_t pos[256];
        size_t total = 0;
        for (int d = 0; d < 256; d++) {
            pos[d] = total;
            total += count[d];
        }
        for (size_t i = 0; i < n; i++) {
            to[pos[(from[i] >> shift) & 255]++] = from[i];
        }
        int64_t *t = from;
        from = to;
        to = t;
    }
    free(buf);
}

bool run_sort(const Args *args, Result *out, SortFn sort) {
    size_t n = args->size;
    int64_t *input = ints(n);
    int64_t *a = xmalloc(n * sizeof *a);
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(a, input, n * sizeof *a);
        double start = now_ms();
        sort(a, n, args->threads);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, h31(a, n));
    free(a);
    free(input);
    return true;
}

static bool sort_int(const Args *args, Result *out) {
    return run_sort(args, out, std_sort);
}

static bool sort_quick(const Args *args, Result *out) {
    return run_sort(args, out, quick_sort);
}

static bool sort_merge(const Args *args, Result *out) {
    return run_sort(args, out, merge_sort_all);
}

static bool sort_heap(const Args *args, Result *out) {
    return run_sort(args, out, heap_sort);
}

static bool sort_radix(const Args *args, Result *out) {
    return run_sort(args, out, radix_sort);
}

char **strings(size_t n, char **block) {
    Rng rng = rng_new();
    char **s = xmalloc(n * sizeof *s);
    char *p = *block = xmalloc(n * 17);
    for (size_t i = 0; i < n; i++) {
        uint32_t len = 8 + rng_next(&rng) % 9;
        s[i] = p;
        for (uint32_t c = 0; c < len; c++) {
            *p++ = (char)('a' + rng_next(&rng) % 26);
        }
        *p++ = '\0';
    }
    return s;
}

int compare_str(const void *pa, const void *pb) {
    return strcmp(*(char *const *)pa, *(char *const *)pb);
}

static bool sort_str(const Args *args, Result *out) {
    size_t n = args->size;
    char *block;
    char **input = strings(n, &block);
    char **s = xmalloc(n * sizeof *s);
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(s, input, n * sizeof *s);
        double start = now_ms();
        qsort(s, n, sizeof *s, compare_str);
        out->times[r] = now_ms() - start;
    }
    Bytes text = {0};
    for (size_t i = 0; i < n; i++) {
        bytes_append(&text, s[i], strlen(s[i]));
        bytes_append(&text, "\n", 1);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(text.data, text.len));
    free(text.data);
    free(s);
    free(input);
    free(block);
    return true;
}

const Kernel SORT_KERNELS[] = {
    {"sort-int", sort_int},
    {"sort-str", sort_str},
    {"quicksort", sort_quick},
    {"mergesort", sort_merge},
    {"heapsort", sort_heap},
    {"radixsort", sort_radix},
    {NULL, NULL},
};
