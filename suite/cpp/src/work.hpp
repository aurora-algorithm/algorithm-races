// The "work" kernels (../SPEC.md section 11), one file per library.
#pragma once

#include <string>

#include "kernels.hpp"

namespace kernels {

Outcome image_kernel(const Args& args);      // image.cpp (stb)
Outcome gzip_kernel(const Args& args);       // compress.cpp (zlib)
Outcome gunzip_kernel(const Args& args);     // compress.cpp
Outcome zstd_kernel(const Args& args);       // compress.cpp (libzstd)
Outcome unzstd_kernel(const Args& args);     // compress.cpp
Outcome bcrypt_kernel(const Args& args);     // passwords.cpp (Botan)
Outcome argon2id_kernel(const Args& args);   // passwords.cpp
Outcome csv_kernel(const Args& args);        // csv.cpp (vincentlaucsb/csv-parser)
Outcome csv_alt_kernel(const Args& args);    // csv_alt.cpp (fast-cpp-csv-parser)
Outcome wordcount_kernel(const Args& args);  // work.cpp

// Libraries and their versions, for the `runtime` field.
std::string zlib_library_version();
std::string zstd_library_version();

}  // namespace kernels
