// Race: bcrypt (c)
// Extracted from suite/c/src/work_password.c — build and run via suite/run.py

/* Work kernels bcrypt (Openwall crypt_blowfish) and argon2id (the reference implementation),
 * SPEC.md section 11. Salts come from the operating system's random source. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argon2.h>
#include <crypt_blowfish.h>
#include "kernels.h"

#define BCRYPT_COST 10

#define BCRYPT_LEN 61 /* 60 characters and the NUL */

#define SALT_LEN 16

#define REF_PASSWORD "correct horse battery staple"

#define BCRYPT_REF "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"

static void random_bytes(void *buf, size_t n) {
#ifdef _WIN32
    bool ok = BCryptGenRandom(NULL, buf, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    bool ok = getentropy(buf, n) == 0;
#endif
    if (!ok) {
        fprintf(stderr, "error: no random bytes from the operating system\n");
        exit(EXIT_FAILURE);
    }
}

static void password(size_t i, char *out, size_t cap) {
    snprintf(out, cap, "user-%zu-password", i);
}

static bool bcrypt_hash(const char *pw, char *hash) {
    char salt[SALT_LEN];
    char setting[30];
    random_bytes(salt, sizeof salt);
    if (_crypt_gensalt_blowfish_rn("$2b$", BCRYPT_COST, salt, sizeof salt, setting,
                                   sizeof setting) == NULL ||
        _crypt_blowfish_rn(pw, setting, hash, BCRYPT_LEN) == NULL) {
        fprintf(stderr, "error: crypt_blowfish failed\n");
        return false;
    }
    return true;
}

static bool bcrypt_verifies(const char *pw, const char *hash) {
    char again[BCRYPT_LEN];
    return _crypt_blowfish_rn(pw, hash, again, sizeof again) != NULL && strcmp(again, hash) == 0;
}

static bool bcrypt(const Args *args, Result *out) {
    size_t n = args->size;
    char (*hashes)[BCRYPT_LEN] = xmalloc(n * sizeof *hashes);
    char pw[64];
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        for (size_t i = 0; i < n; i++) {
            password(i, pw, sizeof pw);
            if (!bcrypt_hash(pw, hashes[i])) {
                free(hashes);
                return false;
            }
        }
        out->times[r] = now_ms() - start;
    }
    size_t ok = 0;
    for (size_t i = 0; i < n; i++) {
        password(i, pw, sizeof pw);
        ok += strncmp(hashes[i] + 3, "$10$", 4) == 0 && bcrypt_verifies(pw, hashes[i]);
    }
    int reference = bcrypt_verifies(REF_PASSWORD, BCRYPT_REF);
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%d", n, ok, reference);
    free(hashes);
    return true;
}
