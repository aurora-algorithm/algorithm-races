/* The string kernels of group `puzzle` (SPEC.md section 13): palindrome, bwt, aho-corasick,
 * huffman. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"
#include "puzzle.h"

static const unsigned char DNA[4] = {'A', 'C', 'G', 'T'};

/* palindrome: Manacher's mirror trick finds every palindrome in linear time */

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

/* bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string */

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

/* qsort has no context argument in C17 (qsort_s and qsort_r differ between MSVC, glibc and macOS),
 * so the comparator reads the two rank arrays of the current round from here. */
static const int32_t *sort_rank;
static const int32_t *sort_second;

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

/* aho-corasick: a trie with failure links matches thousands of patterns in one pass */

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

/* huffman: merge the two rarest symbols until one tree is left */

/* A 1 MiB block of letters skewed toward 'a' (two generator values per letter), repeated. */
static unsigned char *skewed_text(size_t n, size_t *len) {
    *len = n / ONE_MIB * ONE_MIB;
    unsigned char *text = xmalloc(*len ? *len : 1);
    Rng rng = rng_new();
    for (size_t i = 0; i < ONE_MIB && i < *len; i++) {
        uint32_t a = rng_next(&rng) % 26;
        uint32_t b = rng_next(&rng) % 26;
        text[i] = (unsigned char)(97 + a * b / 26);
    }
    for (size_t off = ONE_MIB; off < *len; off += ONE_MIB) {
        memcpy(text + off, text, ONE_MIB);
    }
    return text;
}

/* Children of the internal nodes 256, 257, ...; ids below 256 are leaves (the byte itself). */
typedef struct {
    size_t left[512], right[512];
    size_t root;
} Tree;

static void huffman_tree(const uint64_t freq[256], Tree *tree) {
    U64Heap heap = {0};
    for (uint64_t sym = 0; sym < 256; sym++) {
        if (freq[sym] > 0) {
            u64heap_push(&heap, freq[sym] * 1024 + sym);
        }
    }
    size_t next_id = 256;
    while (heap.len > 1) {
        uint64_t a = u64heap_pop(&heap);
        uint64_t b = u64heap_pop(&heap);
        tree->left[next_id] = (size_t)(a % 1024);
        tree->right[next_id] = (size_t)(b % 1024);
        u64heap_push(&heap, (a / 1024 + b / 1024) * 1024 + next_id);
        next_id++;
    }
    tree->root = heap.len > 0 ? (size_t)(heap.keys[0] % 1024) : 0;
    free(heap.keys);
}

static void huffman_codes(const Tree *tree, uint64_t codes[256], unsigned lengths[256]) {
    struct {
        size_t node;
        uint64_t code;
        unsigned len;
    } stack[512];
    size_t depth = 0;
    stack[depth].node = tree->root;
    stack[depth].code = 0;
    stack[depth++].len = 0;
    while (depth > 0) {
        depth--;
        size_t node = stack[depth].node;
        uint64_t code = stack[depth].code;
        unsigned len = stack[depth].len;
        if (node < 256) {
            codes[node] = code;
            lengths[node] = len;
        } else {
            stack[depth].node = tree->left[node];
            stack[depth].code = code << 1;
            stack[depth++].len = len + 1;
            stack[depth].node = tree->right[node];
            stack[depth].code = (code << 1) | 1;
            stack[depth++].len = len + 1;
        }
    }
}

static uint64_t huffman_encode(const unsigned char *text, size_t n, const uint64_t codes[256],
                               const unsigned lengths[256], Bytes *encoded) {
    uint64_t acc = 0, bits = 0;
    unsigned nbits = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned len = lengths[text[i]];
        acc = (acc << len) | codes[text[i]];
        nbits += len;
        bits += len;
        while (nbits >= 8) {
            nbits -= 8;
            bytes_push(encoded, (unsigned char)(acc >> nbits));
        }
        acc &= ((uint64_t)1 << nbits) - 1;
    }
    if (nbits > 0) {
        bytes_push(encoded, (unsigned char)(acc << (8 - nbits)));
    }
    return bits;
}

static unsigned char *huffman_decode(const Tree *tree, const Bytes *encoded, size_t n) {
    unsigned char *decoded = xcalloc(n, 1);
    size_t len = 0;
    size_t node = tree->root;
    for (size_t i = 0; i < encoded->len && len < n; i++) {
        for (int shift = 7; shift >= 0 && len < n; shift--) {
            node = ((encoded->data[i] >> shift) & 1) ? tree->right[node] : tree->left[node];
            if (node < 256) {
                decoded[len++] = (unsigned char)node;
                node = tree->root;
            }
        }
    }
    return decoded;
}

typedef struct {
    uint64_t bits;
    Bytes encoded;
    unsigned char *decoded;
} HuffmanResult;

static HuffmanResult huffman_run(const unsigned char *text, size_t n) {
    uint64_t freq[256] = {0};
    for (size_t i = 0; i < n; i++) {
        freq[text[i]]++;
    }
    Tree *tree = xmalloc(sizeof *tree);
    huffman_tree(freq, tree);
    uint64_t codes[256] = {0};
    unsigned lengths[256] = {0};
    huffman_codes(tree, codes, lengths);
    Bytes encoded = {0};
    uint64_t bits = huffman_encode(text, n, codes, lengths, &encoded);
    unsigned char *decoded = huffman_decode(tree, &encoded, n);
    free(tree);
    return (HuffmanResult){bits, encoded, decoded};
}

static bool huffman(const Args *args, Result *out) {
    size_t n;
    unsigned char *text = skewed_text(args->size, &n);
    HuffmanResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(res.encoded.data);
        free(res.decoded);
        double start = now_ms();
        res = huffman_run(text, n);
        out->times[r] = now_ms() - start;
    }
    int ok = memcmp(res.decoded, text, n) == 0;
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu32 ":%d", n, res.bits,
             fnv1a(res.encoded.data, res.encoded.len), ok);
    free(res.encoded.data);
    free(res.decoded);
    free(text);
    return true;
}

const Kernel TEXT_KERNELS[] = {
    {"palindrome", palindrome}, {"bwt", bwt}, {"aho-corasick", aho_corasick},
    {"huffman", huffman},       {NULL, NULL},
};
