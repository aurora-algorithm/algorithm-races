/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "kernels.h"

void u64heap_push(U64Heap *h, uint64_t key) {
    if (h->len == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 16;
        h->keys = xrealloc(h->keys, h->cap * sizeof *h->keys);
    }
    size_t i = h->len++;
    while (i > 0 && key < h->keys[(i - 1) / 2]) {
        h->keys[i] = h->keys[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    h->keys[i] = key;
}

uint64_t u64heap_pop(U64Heap *h) {
    uint64_t top = h->keys[0];
    uint64_t last = h->keys[--h->len];
    size_t i = 0;
    for (;;) {
        size_t child = 2 * i + 1;
        if (child >= h->len) {
            break;
        }
        if (child + 1 < h->len && h->keys[child + 1] < h->keys[child]) {
            child++;
        }
        if (h->keys[child] >= last) {
            break;
        }
        h->keys[i] = h->keys[child];
        i = child;
    }
    if (h->len > 0) {
        h->keys[i] = last;
    }
    return top;
}

/* bloom: k bit positions per key from two hashes */

#define BLOOM_BITS_PER_KEY 10
#define BLOOM_HASHES 7u
#define GOLDEN 0x9e3779b9u

typedef struct {
    size_t set_bits, false_positives;
} BloomResult;

static BloomResult bloom_run(const int64_t *keys, const int64_t *lookups, size_t n) {
    uint32_t m = (uint32_t)(BLOOM_BITS_PER_KEY * n);
    unsigned char *bits = xcalloc((m + 7) / 8, 1);
    for (size_t k = 0; k < n; k++) {
        uint32_t h1 = fmix32((uint32_t)keys[k]);
        uint32_t h2 = fmix32((uint32_t)keys[k] ^ GOLDEN) | 1;
        for (uint32_t i = 0; i < BLOOM_HASHES; i++) {
            uint32_t b = (h1 + i * h2) % m;
            bits[b >> 3] |= (unsigned char)(1u << (b & 7));
        }
    }
    size_t false_positives = 0;
    for (size_t k = 0; k < n; k++) {
        uint32_t h1 = fmix32((uint32_t)lookups[k]);
        uint32_t h2 = fmix32((uint32_t)lookups[k] ^ GOLDEN) | 1;
        bool all = true;
        for (uint32_t i = 0; i < BLOOM_HASHES && all; i++) {
            uint32_t b = (h1 + i * h2) % m;
            all = (bits[b >> 3] & (1u << (b & 7))) != 0;
        }
        false_positives += all;
    }
    size_t set_bits = 0;
    for (size_t i = 0; i < (m + 7) / 8; i++) {
        for (unsigned v = bits[i]; v != 0; v &= v - 1) {
            set_bits++;
        }
    }
    free(bits);
    return (BloomResult){set_bits, false_positives};
}

static bool bloom(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *values = ints(2 * n);
    BloomResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = bloom_run(values, values + n, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%zu", n, res.set_bits, res.false_positives);
    free(values);
    return true;
}

/* hyperloglog: the longest run of leading zeros estimates how many distinct items went by */

#define HLL_P 14
#define HLL_M (1u << HLL_P)

/* C17 has no count-leading-zeros (C23's stdc_leading_zeros is not in MSVC 14.38), so the compilers'
 * own intrinsic, as Rust's u32::leading_zeros and Go's bits.LeadingZeros32 compile to. w is not 0.
 */
static unsigned leading_zeros(uint32_t w) {
#ifdef _MSC_VER
    unsigned long index;
    _BitScanReverse(&index, w);
    return 31u - (unsigned)index;
#else
    return (unsigned)__builtin_clz(w);
#endif
}

/* Fills the registers and returns the estimate. */
static double hyperloglog_run(size_t n, unsigned char *registers) {
    Rng rng = rng_new();
    uint32_t universe = (uint32_t)(n / 4 > 1 ? n / 4 : 1);
    memset(registers, 0, HLL_M);
    for (size_t i = 0; i < n; i++) {
        uint32_t h = fmix32(rng_next(&rng) % universe);
        uint32_t w = h << HLL_P;
        unsigned rank = w == 0 ? 32 - HLL_P + 1 : leading_zeros(w) + 1;
        uint32_t idx = h >> (32 - HLL_P);
        if (rank > registers[idx]) {
            registers[idx] = (unsigned char)rank;
        }
    }
    double z = 0.0;
    size_t zeros = 0;
    for (uint32_t i = 0; i < HLL_M; i++) {
        z += pow(2.0, -(double)registers[i]);
        zeros += registers[i] == 0;
    }
    double m = (double)HLL_M;
    double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if (estimate <= 2.5 * m && zeros > 0) {
        estimate = m * log(m / (double)zeros);
    }
    return estimate;
}

static bool hyperloglog(const Args *args, Result *out) {
    unsigned char *registers = xmalloc(HLL_M);
    double estimate = 0.0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        estimate = hyperloglog_run(args->size, registers);
        out->times[r] = now_ms() - start;
    }
    uint32_t hash = 0;
    for (uint32_t i = 0; i < HLL_M; i++) {
        hash = h31_step(hash, registers[i]);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%.0f:%" PRIu32, args->size, floor(estimate + 0.5),
             hash);
    free(registers);
    return true;
}

/* skiplist: towers of random height turn a linked list into a search tree */

#define SKIP_MAX_LEVEL 24
#define SKIP_LEVEL_SEED 88675123u

typedef struct SkipNode {
    int64_t key;
    struct SkipNode *next[]; /* one link per level of this node's tower */
} SkipNode;

typedef struct {
    SkipNode *head;
    size_t level, hits, heights;
} SkipList;

static SkipNode *skip_node(int64_t key, size_t height) {
    SkipNode *node = xmalloc(sizeof *node + height * sizeof(SkipNode *));
    node->key = key;
    return node;
}

/* Walks from the top level down to the last node with a smaller key, noting where it dropped a
 * level. */
static SkipNode *skip_find(const SkipList *list, int64_t key, SkipNode **update) {
    SkipNode *x = list->head;
    for (size_t i = list->level; i-- > 0;) {
        while (x->next[i] != NULL && x->next[i]->key < key) {
            x = x->next[i];
        }
        update[i] = x;
    }
    SkipNode *next = x->next[0];
    return next != NULL && next->key == key ? next : NULL;
}

static void skip_insert(SkipList *list, int64_t key, Rng *rng) {
    SkipNode *update[SKIP_MAX_LEVEL];
    if (skip_find(list, key, update) != NULL) {
        return;
    }
    size_t height = 1;
    while (height < SKIP_MAX_LEVEL && (rng_next(rng) & 1) != 0) {
        height++;
    }
    for (size_t i = list->level; i < height; i++) {
        update[i] = list->head;
    }
    if (height > list->level) {
        list->level = height;
    }
    SkipNode *node = skip_node(key, height);
    for (size_t i = 0; i < height; i++) {
        node->next[i] = update[i]->next[i];
        update[i]->next[i] = node;
    }
    list->heights += height;
}

static SkipList skiplist_build(const int64_t *keys, const int64_t *lookups, size_t n) {
    SkipList list = {skip_node(-1, SKIP_MAX_LEVEL), 1, 0, 0};
    for (int i = 0; i < SKIP_MAX_LEVEL; i++) {
        list.head->next[i] = NULL;
    }
    Rng rng = {SKIP_LEVEL_SEED};
    for (size_t i = 0; i < n; i++) {
        skip_insert(&list, keys[i], &rng);
    }
    SkipNode *update[SKIP_MAX_LEVEL];
    for (size_t i = 0; i < n; i++) {
        list.hits += skip_find(&list, lookups[i], update) != NULL;
    }
    return list;
}

/* h31 of the keys along level 0, freeing the nodes on the way. */
static uint32_t skiplist_free(SkipList *list) {
    uint32_t h = 0;
    SkipNode *x = list->head->next[0];
    free(list->head);
    while (x != NULL) {
        SkipNode *next = x->next[0];
        h = h31_step(h, x->key);
        free(x);
        x = next;
    }
    return h;
}

static bool skiplist(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *keys, *lookups;
    hashmap_input(n, &keys, &lookups);
    SkipList list = {0};
    uint32_t h = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        list = skiplist_build(keys, lookups, n);
        out->times[r] = now_ms() - start;
        h = skiplist_free(&list);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%zu:%" PRIu32, n, list.hits, list.heights, h);
    free(keys);
    free(lookups);
    return true;
}

/* cycle: Floyd's tortoise and hare on x -> x*x + c mod M */

#define CYCLE_MOD 67108859u
#define CYCLE_C 12345u

static uint64_t step(uint64_t x) {
    return (x * x + CYCLE_C) % CYCLE_MOD;
}

typedef struct {
    uint64_t total_mu, total_lam, longest;
} CycleResult;

static CycleResult cycle_run(const uint64_t *starts, size_t n) {
    uint64_t total_mu = 0, total_lam = 0, longest = 0;
    for (size_t i = 0; i < n; i++) {
        uint64_t x0 = starts[i];
        uint64_t tortoise = step(x0);
        uint64_t hare = step(step(x0));
        while (tortoise != hare) {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        uint64_t mu = 0;
        tortoise = x0;
        while (tortoise != hare) {
            tortoise = step(tortoise);
            hare = step(hare);
            mu++;
        }
        uint64_t lam = 1;
        hare = step(tortoise);
        while (tortoise != hare) {
            hare = step(hare);
            lam++;
        }
        total_mu += mu;
        total_lam += lam;
        longest = mu + lam > longest ? mu + lam : longest;
    }
    return (CycleResult){total_mu, total_lam, longest};
}

static bool cycle(const Args *args, Result *out) {
    size_t n = args->size;
    uint64_t *starts = xmalloc(n * sizeof *starts);
    Rng rng = rng_new();
    for (size_t i = 0; i < n; i++) {
        starts[i] = rng_next(&rng) % CYCLE_MOD;
    }
    CycleResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = cycle_run(starts, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64 ":%" PRIu64, n, res.total_mu,
             res.total_lam, res.longest);
    free(starts);
    return true;
}

/* life: Conway's Game of Life on a torus */

#define LIFE_SIDE 512

static unsigned char *life_run(const unsigned char *grid, size_t generations) {
    const size_t w = LIFE_SIDE;
    unsigned char *cur = xmalloc(w * w);
    unsigned char *next = xmalloc(w * w);
    memcpy(cur, grid, w * w);
    for (size_t g = 0; g < generations; g++) {
        for (size_t y = 0; y < w; y++) {
            size_t up = (y + w - 1) % w * w;
            size_t row = y * w;
            size_t down = (y + 1) % w * w;
            for (size_t x = 0; x < w; x++) {
                size_t left = (x + w - 1) % w;
                size_t right = (x + 1) % w;
                unsigned n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] +
                             cur[row + right] + cur[down + left] + cur[down + x] +
                             cur[down + right];
                next[row + x] = (unsigned char)(n == 3 || (n == 2 && cur[row + x] == 1));
            }
        }
        unsigned char *t = cur;
        cur = next;
        next = t;
    }
    free(next);
    return cur;
}

static bool life(const Args *args, Result *out) {
    const size_t cells = LIFE_SIDE * LIFE_SIDE;
    unsigned char *grid = xmalloc(cells);
    Rng rng = rng_new();
    for (size_t i = 0; i < cells; i++) {
        grid[i] = (unsigned char)(rng_next(&rng) % 4 == 0);
    }
    unsigned char *result = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(result);
        double start = now_ms();
        result = life_run(grid, args->size);
        out->times[r] = now_ms() - start;
    }
    size_t alive = 0;
    for (size_t i = 0; i < cells; i++) {
        alive += result[i];
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, args->size, alive,
             fnv1a(result, cells));
    free(result);
    free(grid);
    return true;
}

/* window-max: a monotonic deque keeps the maximum of every window in O(1) per step */

static const size_t WINDOWS[5] = {10, 100, 1000, 10000, 100000};

/* The h31 of each window size's maxima is folded in as they come: keeping every maximum for after
 * the timer would take 5 x N values of memory (see NOTES.md). */
static void window_max_run(const int64_t *values, size_t n, uint32_t hashes[5]) {
    size_t *dq = xmalloc(n * sizeof *dq);
    for (int k = 0; k < 5; k++) {
        size_t w = WINDOWS[k];
        size_t head = 0, tail = 0;
        uint32_t h = 0;
        for (size_t i = 0; i < n; i++) {
            while (tail > head && values[dq[tail - 1]] <= values[i]) {
                tail--;
            }
            dq[tail++] = i;
            if (dq[head] + w <= i) {
                head++;
            }
            if (i + 1 >= w) {
                h = h31_step(h, values[dq[head]]);
            }
        }
        hashes[k] = h;
    }
    free(dq);
}

static bool window_max(const Args *args, Result *out) {
    int64_t *values = ints(args->size);
    uint32_t h[5] = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        window_max_run(values, args->size, h);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX,
             "%zu:%" PRIu32 ":%" PRIu32 ":%" PRIu32 ":%" PRIu32 ":%" PRIu32, args->size, h[0], h[1],
             h[2], h[3], h[4]);
    free(values);
    return true;
}

/* rain-water: flood a 2D terrain from its walls inward with a min-heap */

typedef struct {
    uint64_t water, wet;
} RainResult;

static RainResult rain_water_run(size_t side, const uint64_t *heights) {
    size_t n = side * side;
    bool *seen = xcalloc(n, sizeof *seen);
    U64Heap heap = {0};
    for (size_t idx = 0; idx < n; idx++) {
        size_t r = idx / side, c = idx % side;
        if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
            seen[idx] = true;
            u64heap_push(&heap, heights[idx] * n + idx);
        }
    }
    uint64_t water = 0, wet = 0;
    while (heap.len > 0) {
        uint64_t key = u64heap_pop(&heap);
        uint64_t level = key / n;
        size_t idx = (size_t)(key % n);
        size_t r = idx / side, c = idx % side;
        const struct {
            size_t next;
            bool ok;
        } neighbours[4] = {{idx - side, r > 0},
                           {idx + side, r + 1 < side},
                           {idx - 1, c > 0},
                           {idx + 1, c + 1 < side}};
        for (int k = 0; k < 4; k++) {
            size_t next = neighbours[k].next;
            if (!neighbours[k].ok || seen[next]) {
                continue;
            }
            seen[next] = true;
            uint64_t h = heights[next];
            if (h < level) {
                water += level - h;
                wet++;
                h = level;
            }
            u64heap_push(&heap, h * n + next);
        }
    }
    free(heap.keys);
    free(seen);
    return (RainResult){water, wet};
}

static bool rain_water(const Args *args, Result *out) {
    size_t side = args->size;
    uint64_t *heights = xmalloc(side * side * sizeof *heights);
    Rng rng = rng_new();
    for (size_t i = 0; i < side * side; i++) {
        heights[i] = rng_next(&rng) % 1000;
    }
    RainResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = rain_water_run(side, heights);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64, side, res.water, res.wet);
    free(heights);
    return true;
}

/* collatz: remember every chain length below the start so each walk stops early */

typedef struct {
    size_t best;
    uint32_t best_len;
    uint64_t total;
} CollatzResult;

static CollatzResult collatz_run(size_t n) {
    uint32_t *cache = xcalloc(n, sizeof *cache);
    cache[1] = 1;
    size_t best = 1;
    uint32_t best_len = 1;
    uint64_t total = 1;
    for (size_t start = 2; start < n; start++) {
        uint64_t x = start;
        uint32_t steps = 0;
        while (x >= start) {
            x = x % 2 == 0 ? x / 2 : 3 * x + 1;
            steps++;
        }
        uint32_t len = steps + cache[x];
        cache[start] = len;
        total += len;
        if (len > best_len) {
            best = start;
            best_len = len;
        }
    }
    free(cache);
    return (CollatzResult){best, best_len, total};
}

static bool collatz(const Args *args, Result *out) {
    CollatzResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = collatz_run(args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32 ":%" PRIu64, args->size, res.best,
             res.best_len, res.total);
    return true;
}

const Kernel PUZZLE_KERNELS[] = {
    {"bloom", bloom},
    {"hyperloglog", hyperloglog},
    {"skiplist", skiplist},
    {"cycle", cycle},
    {"life", life},
    {"window-max", window_max},
    {"rain-water", rain_water},
    {"collatz", collatz},
    {NULL, NULL},
};
