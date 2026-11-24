// Race: gzip (c)
// Extracted from suite/c/src/work_compress.c — build and run via suite/run.py

/* Work kernels gzip, gunzip (zlib) and zstd, unzstd (libzstd) of SPEC.md section 11. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbedtls/sha256.h>
#include <zlib.h>
#include <zstd.h>
#include "kernels.h"

#define GZIP_LEVEL 6

#define GZIP_WINDOW (15 + 16) /* 15-bit window, +16 asks zlib for a gzip header and trailer */

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

static bool gzip_compress(const unsigned char *raw, size_t len, Bytes *packed) {
    z_stream s = {0};
    if (deflateInit2(&s, GZIP_LEVEL, Z_DEFLATED, GZIP_WINDOW, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        fprintf(stderr, "error: deflateInit2 failed\n");
        return false;
    }
    packed->len = 0;
    bytes_reserve(packed, deflateBound(&s, (uLong)len));
    s.next_in = (Bytef *)raw;
    s.avail_in = (uInt)len;
    s.next_out = packed->data;
    s.avail_out = (uInt)packed->cap;
    int rc = deflate(&s, Z_FINISH);
    packed->len = s.total_out;
    deflateEnd(&s);
    if (rc != Z_STREAM_END) {
        fprintf(stderr, "error: deflate returned %d\n", rc);
        return false;
    }
    return true;
}

static bool gzip_decompress(const unsigned char *packed, size_t len, Bytes *raw) {
    z_stream s = {0};
    if (inflateInit2(&s, GZIP_WINDOW) != Z_OK) {
        fprintf(stderr, "error: inflateInit2 failed\n");
        return false;
    }
    raw->len = 0;
    s.next_in = (Bytef *)packed;
    s.avail_in = (uInt)len;
    int rc = Z_OK;
    while (rc == Z_OK) {
        bytes_reserve(raw, raw->len < 65536 ? 65536 : raw->len);
        s.next_out = raw->data + raw->len;
        s.avail_out = (uInt)(raw->cap - raw->len);
        rc = inflate(&s, Z_NO_FLUSH);
        raw->len = s.total_out;
    }
    inflateEnd(&s);
    if (rc != Z_STREAM_END) {
        fprintf(stderr, "error: inflate returned %d\n", rc);
        return false;
    }
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

static bool gzip(const Args *args, Result *out) {
    return run_pack(args, out, gzip_compress, gzip_decompress);
}
