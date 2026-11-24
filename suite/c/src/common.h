/* Shared pieces of the C kernels (../../SPEC.md section 2): arguments, results, the generator,
 * checksums, the clock and checked allocation. */
#ifndef KERNELS_COMMON_H
#define KERNELS_COMMON_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHECKSUM_MAX 256
#define EXTRA_MAX 512

typedef struct {
    const char *kernel;
    size_t size;
    size_t repeats;
    size_t threads;
    const char *data;
} Args;

/* What a kernel hands back: the checksum, one time per repeat (ms), optional `extra` JSON object.
 */
typedef struct {
    char checksum[CHECKSUM_MAX];
    double *times;
    char extra[EXTRA_MAX];
} Result;

/* Returns false after printing the reason to stderr. */
typedef bool (*KernelFn)(const Args *args, Result *out);

typedef struct {
    const char *name;
    KernelFn run;
} Kernel;

/* Marsaglia's xorshift32; every input starts from a fresh generator. */
typedef struct {
    uint32_t state;
} Rng;

#define SEED 2463534242u

static inline Rng rng_new(void) {
    return (Rng){SEED};
}

static inline uint32_t rng_next(Rng *rng) {
    uint32_t s = rng->state;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    rng->state = s;
    return s;
}

/* h = h * 31 + v modulo 2^32; only the low 32 bits of each value matter. */
static inline uint32_t h31_step(uint32_t h, int64_t v) {
    return h * 31u + (uint32_t)v;
}

uint32_t h31(const int64_t *values, size_t n);
uint32_t fnv1a(const void *bytes, size_t n);

/* The kernels' results are written here so the compiler cannot drop a repeat whose result goes
 * unused. */
extern volatile uint64_t sink;

double now_ms(void);

void *xmalloc(size_t bytes);
void *xcalloc(size_t count, size_t bytes);
void *xrealloc(void *p, size_t bytes);
char *path_join(const char *dir, const char *name);
/* Reads a whole file; NULL (and a message on stderr) when it cannot. */
unsigned char *read_file(const char *path, size_t *len);

/* A growable byte buffer. */
typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} Bytes;

void bytes_reserve(Bytes *b, size_t extra);
void bytes_append(Bytes *b, const void *src, size_t n);
void bytes_printf(Bytes *b, const char *fmt, ...);

static inline void bytes_push(Bytes *b, unsigned char byte) {
    if (b->len == b->cap) {
        bytes_reserve(b, 1);
    }
    b->data[b->len++] = byte;
}

/* Inputs of SPEC.md section 2. */
int64_t *ints(size_t n);
/* hashmap(N): inserts = ints(N), lookups[i] = inserts[i] ^ (i & 1). */
void hashmap_input(size_t n, int64_t **inserts, int64_t **lookups);

#endif
