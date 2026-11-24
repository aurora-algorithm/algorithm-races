// Race: palindrome (c)
// Extracted from suite/c/src/puzzle_text.c — build and run via suite/run.py

/* The string kernels of group `puzzle` (SPEC.md section 13): palindrome, bwt, aho-corasick,
 * huffman. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"
#include "puzzle.h"

/* 'a'/'b' from the bits of generator values, low bit first, 32 characters per value. */
static unsigned char *binary_text(size_t n) {
    Rng rng = rng_new();
    unsigned char *out = xmalloc(n);
    for (size_t i = 0; i < n; i += 32) {
        uint32_t v = rng_next(&rng);
        for (size_t k = i; k < n && k < i + 32; k++) {
            out[k] = (v & 1) ? 'b' : 'a';
            v >>= 1;
        }
    }
    return out;
}

typedef struct {
    size_t longest, start, count;
} PalindromeResult;

static PalindromeResult palindrome_run(const unsigned char *text, size_t n) {
    size_t len = 2 * n + 3, last = len - 1;
    unsigned char *t = xmalloc(len);
    memset(t, '#', len);
    t[0] = '^';
    t[last] = '$';
    for (size_t i = 0; i < n; i++) {
        t[2 + 2 * i] = text[i];
    }
    size_t *p = xcalloc(len, sizeof *p);
    size_t center = 0, right = 0;
    for (size_t i = 1; i < last; i++) {
        if (i < right) {
            size_t mirror = p[2 * center - i];
            p[i] = right - i < mirror ? right - i : mirror;
        }
        while (t[i + 1 + p[i]] == t[i - 1 - p[i]]) {
            p[i]++;
        }
        if (i + p[i] > right) {
            center = i;
            right = i + p[i];
        }
    }
    size_t longest = 0, best = 0, count = 0;
    for (size_t i = 0; i < len; i++) {
        count += (p[i] + 1) / 2;
        if (p[i] > longest) {
            longest = p[i];
            best = i;
        }
    }
    free(p);
    free(t);
    return (PalindromeResult){longest, (best - 1 - longest) / 2, count};
}

static bool palindrome(const Args *args, Result *out) {
    unsigned char *text = binary_text(args->size);
    PalindromeResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = palindrome_run(text, args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%zu:%zu", args->size, res.longest, res.start,
             res.count);
    free(text);
    return true;
}
