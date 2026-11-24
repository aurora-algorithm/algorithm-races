// Race: hashmap (c)
// Extracted from suite/c/src/lib.c — build and run via suite/run.py

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <mbedtls/sha256.h>
#include <pcre2.h>
#include "intmap.h"
#include "kernels.h"

static bool hashmap(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *inserts, *lookups;
    hashmap_input(n, &inserts, &lookups);
    size_t keys = 0, hits = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        IntMap counts = intmap_new();
        for (size_t i = 0; i < n; i++) {
            *intmap_entry(&counts, inserts[i]) += 1;
        }
        hits = 0;
        for (size_t i = 0; i < n; i++) {
            hits += intmap_contains(&counts, lookups[i]);
        }
        keys = counts.len;
        intmap_free(&counts);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu", keys, hits);
    free(inserts);
    free(lookups);
    return true;
}
