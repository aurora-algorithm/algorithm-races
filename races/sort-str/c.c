// Race: sort-str (c)
// Extracted from suite/c/src/sorts.c — build and run via suite/run.py

/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

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
