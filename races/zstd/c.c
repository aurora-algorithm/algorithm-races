// Race: zstd (c)
// Extracted from suite/c/src/work_compress.c — build and run via suite/run.py

/* Work kernels gzip, gunzip (zlib) and zstd, unzstd (libzstd) of SPEC.md section 11. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbedtls/sha256.h>
#include <zlib.h>
#include <zstd.h>
#include "kernels.h"

#define ZSTD_LEVEL 3

static unsigned char *read_corpus(const Args *args, size_t *len) {
    char name[64];
    snprintf(name, sizeof name, "corpus-%zu.bin", args->size);
    char *path = path_join(args->data, name);
    unsigned char *raw = read_file(path, len);
    free(path);
    return raw;
}

/* "{len}:{sha256 hex}" of the decompressed bytes. */
static void plain_checksum(const unsigned char *raw, size_t len, char *checksum) {
    unsigned char digest[32];
    mbedtls_sha256(raw, len, digest, 0);
    int at = snprintf(checksum, CHECKSUM_MAX, "%zu:", len);
    for (int i = 0; i < 32; i++) {
        at += snprintf(checksum + at, CHECKSUM_MAX - (size_t)at, "%02x", digest[i]);
    }
}

static bool zstd_compress(const unsigned char *raw, size_t len, Bytes *packed) {
    packed->len = 0;
    bytes_reserve(packed, ZSTD_compressBound(len));
    size_t got = ZSTD_compress(packed->data, packed->cap, raw, len, ZSTD_LEVEL);
    if (ZSTD_isError(got)) {
        fprintf(stderr, "error: ZSTD_compress: %s\n", ZSTD_getErrorName(got));
        return false;
    }
    packed->len = got;
    return true;
}

/* ZSTD_compress records the content size in the frame header, so one buffer of that size is enough.
 */
static bool zstd_decompress(const unsigned char *packed, size_t len, Bytes *raw) {
    unsigned long long size = ZSTD_getFrameContentSize(packed, len);
    if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
        fprintf(stderr, "error: zstd frame without a content size\n");
        return false;
    }
    raw->len = 0;
    bytes_reserve(raw, (size_t)size);
    size_t got = ZSTD_decompress(raw->data, raw->cap, packed, len);
    if (ZSTD_isError(got)) {
        fprintf(stderr, "error: ZSTD_decompress: %s\n", ZSTD_getErrorName(got));
        return false;
    }
    raw->len = got;
    return true;
}

typedef bool (*Codec)(const unsigned char *in, size_t len, Bytes *out);

/* Times `pack` on the corpus; checksum from unpacking the result after timing. */
static bool run_pack(const Args *args, Result *out, Codec pack, Codec unpack) {
    size_t len;
    unsigned char *raw = read_corpus(args, &len);
    if (raw == NULL) {
        return false;
    }
    bool ok = true;
    Bytes packed = {0}, check = {0};
    for (size_t r = 0; r < args->repeats && ok; r++) {
        free(packed.data);
        packed = (Bytes){0};
        double start = now_ms();
        ok = pack(raw, len, &packed);
        out->times[r] = now_ms() - start;
    }
    ok = ok && unpack(packed.data, packed.len, &check);
    if (ok) {
        plain_checksum(check.data, check.len, out->checksum);
        snprintf(out->extra, EXTRA_MAX, "{\"compressed_bytes\":%zu}", packed.len);
    }
    free(check.data);
    free(packed.data);
    free(raw);
    return ok;
}

static bool zstd(const Args *args, Result *out) {
    return run_pack(args, out, zstd_compress, zstd_decompress);
}
