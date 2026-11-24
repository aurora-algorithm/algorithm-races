#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

volatile uint64_t sink;

uint32_t h31(const int64_t *values, size_t n) {
    uint32_t h = 0;
    for (size_t i = 0; i < n; i++) {
        h = h31_step(h, values[i]);
    }
    return h;
}

uint32_t fnv1a(const void *bytes, size_t n) {
    const unsigned char *p = bytes;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

#ifdef _WIN32
double now_ms(void) {
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
}
#else
double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}
#endif

static void out_of_memory(size_t bytes) {
    fprintf(stderr, "error: out of memory allocating %zu bytes\n", bytes);
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t bytes) {
    void *p = malloc(bytes ? bytes : 1);
    if (p == NULL) {
        out_of_memory(bytes);
    }
    return p;
}

void *xcalloc(size_t count, size_t bytes) {
    void *p = calloc(count ? count : 1, bytes ? bytes : 1);
    if (p == NULL) {
        out_of_memory(count * bytes);
    }
    return p;
}

void *xrealloc(void *p, size_t bytes) {
    void *q = realloc(p, bytes ? bytes : 1);
    if (q == NULL) {
        out_of_memory(bytes);
    }
    return q;
}

char *path_join(const char *dir, const char *name) {
    size_t a = strlen(dir);
    size_t b = strlen(name);
    char *path = xmalloc(a + b + 2);
    memcpy(path, dir, a);
    path[a] = '/';
    memcpy(path + a + 1, name, b + 1);
    return path;
}

unsigned char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open %s\n", path);
        return NULL;
    }
    Bytes b = {0};
    size_t got;
    do {
        bytes_reserve(&b, 1 << 20);
        got = fread(b.data + b.len, 1, b.cap - b.len, f);
        b.len += got;
    } while (got > 0);
    bool failed = ferror(f) != 0;
    fclose(f);
    if (failed) {
        fprintf(stderr, "error: cannot read %s\n", path);
        free(b.data);
        return NULL;
    }
    *len = b.len;
    return b.data;
}

void bytes_reserve(Bytes *b, size_t extra) {
    if (b->len + extra <= b->cap) {
        return;
    }
    size_t cap = b->cap ? b->cap : 64;
    while (cap < b->len + extra) {
        cap *= 2;
    }
    b->data = xrealloc(b->data, cap);
    b->cap = cap;
}

void bytes_append(Bytes *b, const void *src, size_t n) {
    bytes_reserve(b, n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

void bytes_printf(Bytes *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) {
        fprintf(stderr, "error: bad format %s\n", fmt);
        exit(EXIT_FAILURE);
    }
    bytes_reserve(b, (size_t)n + 1);
    va_start(ap, fmt);
    vsnprintf((char *)b->data + b->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    b->len += (size_t)n;
}

int64_t *ints(size_t n) {
    Rng rng = rng_new();
    int64_t *a = xmalloc(n * sizeof *a);
    for (size_t i = 0; i < n; i++) {
        a[i] = rng_next(&rng);
    }
    return a;
}

void hashmap_input(size_t n, int64_t **inserts, int64_t **lookups) {
    *inserts = ints(n);
    *lookups = xmalloc(n * sizeof **lookups);
    for (size_t i = 0; i < n; i++) {
        (*lookups)[i] = (*inserts)[i] ^ (int64_t)(i & 1);
    }
}
