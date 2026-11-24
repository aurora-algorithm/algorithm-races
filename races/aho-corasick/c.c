// Race: aho-corasick (c)
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

#define AHO_PATTERNS 10000

typedef struct {
    unsigned char *letters; /* every pattern back to back */
    size_t offsets[AHO_PATTERNS + 1];
    unsigned char *text;
    size_t text_len;
} AhoInput;

static AhoInput *aho_input(size_t n) {
    Rng rng = rng_new();
    AhoInput *in = xmalloc(sizeof *in);
    in->letters = xmalloc(AHO_PATTERNS * 12);
    size_t pos = 0;
    for (size_t p = 0; p < AHO_PATTERNS; p++) {
        in->offsets[p] = pos;
        uint32_t len = 6 + rng_next(&rng) % 7;
        for (uint32_t i = 0; i < len; i++) {
            in->letters[pos++] = DNA[rng_next(&rng) % 4];
        }
    }
    in->offsets[AHO_PATTERNS] = pos;
    in->text_len = n / ONE_MIB * ONE_MIB;
    in->text = xmalloc(in->text_len ? in->text_len : 1);
    if (in->text_len > 0) {
        dna_from(&rng, in->text, ONE_MIB);
        for (size_t off = ONE_MIB; off < in->text_len; off += ONE_MIB) {
            memcpy(in->text + off, in->text, ONE_MIB);
        }
    }
    return in;
}

typedef struct {
    size_t states;
    uint64_t matches, hits;
} AhoResult;

static AhoResult aho_run(const AhoInput *in) {
    size_t code[256] = {0};
    for (size_t i = 0; i < 4; i++) {
        code[DNA[i]] = i;
    }
    size_t states = 1, cap = 1024;
    int32_t *go = xmalloc(4 * cap * sizeof *go);
    uint64_t *outc = xmalloc(cap * sizeof *outc);
    memset(go, -1, 4 * sizeof *go);
    outc[0] = 0;
    for (size_t p = 0; p < AHO_PATTERNS; p++) {
        size_t s = 0;
        for (size_t i = in->offsets[p]; i < in->offsets[p + 1]; i++) {
            size_t edge = s * 4 + code[in->letters[i]];
            if (go[edge] == -1) {
                if (states == cap) {
                    cap *= 2;
                    go = xrealloc(go, 4 * cap * sizeof *go);
                    outc = xrealloc(outc, cap * sizeof *outc);
                }
                go[edge] = (int32_t)states;
                memset(go + 4 * states, -1, 4 * sizeof *go);
                outc[states++] = 0;
            }
            s = (size_t)go[edge];
        }
        outc[s]++;
    }
    size_t *fail = xcalloc(states, sizeof *fail);
    size_t *queue = xmalloc(states * sizeof *queue);
    size_t tail = 0;
    for (size_t c = 0; c < 4; c++) {
        if (go[c] == -1) {
            go[c] = 0;
        } else {
            queue[tail++] = (size_t)go[c];
        }
    }
    for (size_t head = 0; head < tail; head++) {
        size_t s = queue[head];
        outc[s] += outc[fail[s]];
        for (size_t c = 0; c < 4; c++) {
            int32_t fallback = go[fail[s] * 4 + c];
            int32_t t = go[s * 4 + c];
            if (t == -1) {
                go[s * 4 + c] = fallback;
            } else {
                fail[t] = (size_t)fallback;
                queue[tail++] = (size_t)t;
            }
        }
    }
    size_t s = 0;
    uint64_t matches = 0, hits = 0;
    for (size_t i = 0; i < in->text_len; i++) {
        s = (size_t)go[s * 4 + code[in->text[i]]];
        if (outc[s] > 0) {
            matches += outc[s];
            hits++;
        }
    }
    free(queue);
    free(fail);
    free(outc);
    free(go);
    return (AhoResult){states, matches, hits};
}

static bool aho_corasick(const Args *args, Result *out) {
    AhoInput *in = aho_input(args->size);
    AhoResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = aho_run(in);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%d:%zu:%" PRIu64 ":%" PRIu64, AHO_PATTERNS, res.states,
             res.matches, res.hits);
    free(in->text);
    free(in->letters);
    free(in);
    return true;
}
