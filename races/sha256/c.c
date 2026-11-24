// Race: sha256 (c)
// Extracted from suite/c/src/lib.c — build and run via suite/run.py

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <mbedtls/sha256.h>
#include <pcre2.h>
#include "intmap.h"
#include "kernels.h"

/* One 1 MiB block of generator values (4 bytes each, little-endian), repeated to `size` bytes. */
static unsigned char *sha_buffer(size_t size) {
    const size_t block = (size_t)1 << 20;
    unsigned char *buf = xmalloc(size);
    Rng rng = rng_new();
    for (size_t i = 0; i < block && i < size; i += 4) {
        uint32_t v = rng_next(&rng);
        for (size_t b = 0; b < 4; b++) {
            buf[i + b] = (unsigned char)(v >> (8 * b));
        }
    }
    for (size_t off = block; off + block <= size; off += block) {
        memcpy(buf + off, buf, block);
    }
    return buf;
}

static bool sha256(const Args *args, Result *out) {
    size_t n = args->size;
    unsigned char *buf = sha_buffer(n);
    unsigned char digest[32];
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        int rc = mbedtls_sha256(buf, n, digest, 0);
        out->times[r] = now_ms() - start;
        if (rc != 0) {
            fprintf(stderr, "error: mbedtls_sha256 returned %d\n", rc);
            free(buf);
            return false;
        }
    }
    for (int i = 0; i < 32; i++) {
        snprintf(out->checksum + 2 * i, 3, "%02x", digest[i]);
    }
    free(buf);
    return true;
}
