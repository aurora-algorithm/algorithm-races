#include <string.h>

#include "build_info.h"
#include "kernels.h"

typedef struct {
    const char *kernel;
    const char *libraries;
} KernelLibraries;

static const KernelLibraries USES[] = {
    {"regex-log", LIB_PCRE2 " (interpreter, JIT off: the default build)"},
    {"sha256", LIB_MBEDTLS " (portable C, no SHA-NI)"},
    {"json", LIB_CJSON},
    {"gzip", LIB_ZLIB},
    {"gunzip", LIB_ZLIB},
    {"zstd", LIB_ZSTD},
    {"unzstd", LIB_ZSTD},
    {"bcrypt", LIB_CRYPT_BLOWFISH " (Openwall)"},
    {"argon2id", LIB_ARGON2 " (reference C)"},
    {"csv", LIB_LIBCSV},
    {"image", LIB_STB " (stb_image, stb_image_write), " LIB_STB_RESIZE " (stb_image_resize2)"},
};

const char *kernel_libraries(const char *name) {
    for (size_t i = 0; i < sizeof USES / sizeof USES[0]; i++) {
        if (strcmp(USES[i].kernel, name) == 0) {
            return USES[i].libraries;
        }
    }
    return NULL;
}
