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
namespace {

constexpr std::uint16_t kBcryptCost = 10;
constexpr std::size_t kArgon2M = 19456;
constexpr std::size_t kArgon2T = 2;
constexpr std::size_t kArgon2P = 1;
constexpr std::uint8_t kArgon2id = 2;
constexpr std::string_view kRefPassword = "correct horse battery staple";
constexpr std::string_view kBcryptRef =
    "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";
constexpr std::string_view kArgon2Ref =
    "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$"
    "oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

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

bool argon2_verifies(std::string_view hash, std::string_view password) {
  return Botan::argon2_check_pwhash(password.data(), password.size(), hash);
}

std::vector<std::string> argon2_hash(const std::vector<std::string>& pws) {
  Botan::System_RNG rng;
  std::vector<std::string> hashes;
  hashes.reserve(pws.size());
  for (const auto& pw : pws) {
    hashes.push_back(Botan::argon2_generate_pwhash(pw.data(), pw.size(), rng, kArgon2P, kArgon2M,
                                                   kArgon2T, kArgon2id));
  }
  return hashes;
}

}  // namespace

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

Outcome argon2id_kernel(const Args& args) {
  const auto pws = passwords(args.size);
  auto [hashes, times] = measure(args.repeats, [&] { return argon2_hash(pws); });
  const std::string prefix =
      std::format("$argon2id$v=19$m={},t={},p={}$", kArgon2M, kArgon2T, kArgon2P);
  std::size_t ok = 0;
  for (std::size_t i = 0; i < hashes.size(); ++i) {
    ok += hashes[i].starts_with(prefix) && argon2_verifies(hashes[i], pws[i]) ? 1 : 0;
  }
  const bool reference = argon2_verifies(kArgon2Ref, kRefPassword);
  return {std::format("{}:{}:{}", hashes.size(), ok, reference ? 1 : 0), std::move(times), {}};
}

}  // namespace kernels
