// Race: huffman (c)
// Extracted from suite/c/src/puzzle_text.c — build and run via suite/run.py

/* The string kernels of group `puzzle` (SPEC.md section 13): palindrome, bwt, aho-corasick,
 * huffman. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"
#include "puzzle.h"

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
