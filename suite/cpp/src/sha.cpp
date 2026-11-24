// sha256 with Botan, the C++ standard library has no hash functions (../SPEC.md section 5).
#include <botan_all.h>

#include <memory>
#include <string>

#include "inputs.hpp"
#include "lib.hpp"

namespace kernels {

std::string sha256_hex(std::span<const std::uint8_t> bytes) {
  const auto hash = Botan::HashFunction::create_or_throw("SHA-256");
  hash->update(bytes);
  return Botan::hex_encode(hash->final(), false);
}

Outcome sha256_kernel(const Args& args) {
  const auto buffer = sha_buffer(args.size);
  return outcome(measure(args.repeats, [&] { return sha256_hex(buffer); }));
}

}  // namespace kernels
