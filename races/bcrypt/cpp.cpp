// Race: bcrypt (cpp)
// Extracted from suite/cpp/src/passwords.cpp — build and run via suite/run.py

// bcrypt and argon2id with Botan (../SPEC.md section 11): random salt per password from the system
// RNG.
#include <botan_all.h>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>
#include "work.hpp"

namespace kernels {

constexpr std::uint16_t kBcryptCost = 10;

constexpr std::string_view kRefPassword = "correct horse battery staple";

constexpr std::string_view kBcryptRef =
    "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";

std::vector<std::string> passwords(std::size_t n) {
  std::vector<std::string> out(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = std::format("user-{}-password", i);
  }
  return out;
}

std::vector<std::string> bcrypt_hash(const std::vector<std::string>& pws) {
  Botan::System_RNG rng;
  std::vector<std::string> hashes;
  hashes.reserve(pws.size());
  for (const auto& pw : pws) {
    hashes.push_back(Botan::generate_bcrypt(pw, rng, kBcryptCost, 'b'));
  }
  return hashes;
}

Outcome bcrypt_kernel(const Args& args) {
  const auto pws = passwords(args.size);
  auto [hashes, times] = measure(args.repeats, [&] { return bcrypt_hash(pws); });
  const std::string cost = std::format("${}$", kBcryptCost);
  std::size_t ok = 0;
  for (std::size_t i = 0; i < hashes.size(); ++i) {
    ok += hashes[i].substr(3, 4) == cost && Botan::check_bcrypt(pws[i], hashes[i]) ? 1 : 0;
  }
  const bool reference = Botan::check_bcrypt(kRefPassword, kBcryptRef);
  return {std::format("{}:{}:{}", hashes.size(), ok, reference ? 1 : 0), std::move(times), {}};
}

}  // namespace kernels
