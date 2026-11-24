// Race: bwt (c)
// Extracted from suite/c/src/puzzle_text.c — build and run via suite/run.py

/* The string kernels of group `puzzle` (SPEC.md section 13): palindrome, bwt, aho-corasick,
 * huffman. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"
#include "puzzle.h"

static const unsigned char DNA[4] = {'A', 'C', 'G', 'T'};

/* "ACGT" from 2-bit chunks of generator values, low bits first, 16 characters per value. */
static void dna_from(Rng *rng, unsigned char *out, size_t n) {
    for (size_t i = 0; i < n; i += 16) {
        uint32_t v = rng_next(rng);
        for (size_t k = i; k < n && k < i + 16; k++) {
            out[k] = DNA[v & 3];
            v >>= 2;
        }
    }
}

static int compare_suffix(const void *pa, const void *pb) {
    size_t a = *(const size_t *)pa, b = *(const size_t *)pb;
    if (sort_rank[a] != sort_rank[b]) {
        return sort_rank[a] < sort_rank[b] ? -1 : 1;
    }
    if (sort_second[a] != sort_second[b]) {
        return sort_second[a] < sort_second[b] ? -1 : 1;
    }
    return 0;
}

typedef struct {
    unsigned rounds;
    size_t primary;
    unsigned char *bwt; /* n = dna length + 1 bytes */
} BwtResult;

static BwtResult bwt_run(const unsigned char *dna, size_t dna_len) {
    size_t n = dna_len + 1;
    unsigned char *text = xmalloc(n);
    memcpy(text, dna, dna_len);
    text[dna_len] = '$';
    int32_t *rank = xmalloc(n * sizeof *rank);
    int32_t *second = xmalloc(n * sizeof *second);
    int32_t *tmp = xmalloc(n * sizeof *tmp);
    size_t *sa = xmalloc(n * sizeof *sa);
    for (size_t i = 0; i < n; i++) {
        rank[i] = text[i];
        sa[i] = i;
    }
    size_t k = 1;
    unsigned rounds = 0;
    for (;;) {
        rounds++;
        for (size_t i = 0; i < n; i++) {
            second[i] = i + k < n ? rank[i + k] : -1;
        }
        sort_rank = rank;
        sort_second = second;
        qsort(sa, n, sizeof *sa, compare_suffix);
        tmp[sa[0]] = 0;
        for (size_t j = 1; j < n; j++) {
            size_t a = sa[j - 1], b = sa[j];
            bool differs = rank[a] != rank[b] || second[a] != second[b];
            tmp[b] = tmp[a] + differs;
        }
        int32_t *t = rank;
        rank = tmp;
        tmp = t;
        if ((size_t)rank[sa[n - 1]] == n - 1) {
            break;
        }
        k *= 2;
    }
    unsigned char *last = xmalloc(n);
    size_t primary = 0;
    for (size_t j = 0; j < n; j++) {
        if (sa[j] == 0) {
            primary = j;
            last[j] = text[n - 1];
        } else {
            last[j] = text[sa[j] - 1];
        }
    }
    free(sa);
    free(tmp);
    free(second);
    free(rank);
    free(text);
    return (BwtResult){rounds, primary, last};
}

static bool bwt(const Args *args, Result *out) {
    unsigned char *dna = xmalloc(args->size);
    Rng rng = rng_new();
    dna_from(&rng, dna, args->size);
    BwtResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(res.bwt);
        double start = now_ms();
        res = bwt_run(dna, args->size);
        out->times[r] = now_ms() - start;
    }
    size_t n = args->size + 1;
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%u:%zu:%" PRIu32, n, res.rounds, res.primary,
             fnv1a(res.bwt, n));
    free(res.bwt);
    free(dna);
    return true;
}
