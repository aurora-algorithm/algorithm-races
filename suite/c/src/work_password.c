/* Work kernels bcrypt (Openwall crypt_blowfish) and argon2id (the reference implementation),
 * SPEC.md section 11. Salts come from the operating system's random source. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <bcrypt.h>
#else
#include <sys/random.h>
#endif

#include <argon2.h>
#include <crypt_blowfish.h>

#include "kernels.h"

#define BCRYPT_COST 10
#define BCRYPT_LEN 61 /* 60 characters and the NUL */
#define ARGON2_M 19456
#define ARGON2_T 2
#define ARGON2_P 1
#define SALT_LEN 16
#define HASH_LEN 32
#define REF_PASSWORD "correct horse battery staple"
#define BCRYPT_REF "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"
#define ARGON2_REF                                                                                 \
    "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$"                                      \
    "oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk"
#define ARGON2_PREFIX "$argon2id$v=19$m=19456,t=2,p=1$"

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

static bool argon2_verifies(const char *encoded, const char *pw) {
    return argon2id_verify(encoded, pw, strlen(pw)) == ARGON2_OK;
}

static bool argon2id(const Args *args, Result *out) {
    size_t n = args->size;
    size_t len = argon2_encodedlen(ARGON2_T, ARGON2_M, ARGON2_P, SALT_LEN, HASH_LEN, Argon2_id);
    char *hashes = xmalloc(n * len);
    char pw[64];
    unsigned char salt[SALT_LEN];
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        for (size_t i = 0; i < n; i++) {
            password(i, pw, sizeof pw);
            random_bytes(salt, sizeof salt);
            int rc = argon2id_hash_encoded(ARGON2_T, ARGON2_M, ARGON2_P, pw, strlen(pw), salt,
                                           sizeof salt, HASH_LEN, hashes + i * len, len);
            if (rc != ARGON2_OK) {
                fprintf(stderr, "error: argon2id: %s\n", argon2_error_message(rc));
                free(hashes);
                return false;
            }
        }
        out->times[r] = now_ms() - start;
    }
    size_t ok = 0;
    for (size_t i = 0; i < n; i++) {
        password(i, pw, sizeof pw);
        const char *h = hashes + i * len;
        ok += strncmp(h, ARGON2_PREFIX, strlen(ARGON2_PREFIX)) == 0 && argon2_verifies(h, pw);
    }
    int reference = argon2_verifies(ARGON2_REF, REF_PASSWORD);
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%d", n, ok, reference);
    free(hashes);
    return true;
}

const Kernel PASSWORD_KERNELS[] = {{"bcrypt", bcrypt}, {"argon2id", argon2id}, {NULL, NULL}};
