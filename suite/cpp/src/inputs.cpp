#include "inputs.hpp"

#include <array>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace kernels {
namespace {

constexpr std::uint32_t kFnvOffset = 2166136261u;
constexpr std::uint32_t kFnvPrime = 16777619u;
constexpr std::size_t kShaBlockValues = 1 << 18;
constexpr std::array<std::string_view, 4> kMethods = {"GET", "POST", "PUT", "DELETE"};
constexpr std::array<std::uint32_t, 12> kStatuses = {200, 201, 204, 301, 304, 400,
                                                     401, 403, 404, 500, 502, 503};

}  // namespace

std::uint32_t fnv1a(std::span<const std::uint8_t> bytes) {
  std::uint32_t h = kFnvOffset;
  for (const std::uint8_t b : bytes) {
    h = (h ^ b) * kFnvPrime;
  }
  return h;
}

std::uint32_t fnv1a(std::string_view text) {
  std::uint32_t h = kFnvOffset;
  for (const char c : text) {
    h = (h ^ static_cast<std::uint8_t>(c)) * kFnvPrime;
  }
  return h;
}

std::vector<std::int64_t> ints(std::size_t n) {
  XorShift32 rng;
  std::vector<std::int64_t> out(n);
  for (auto& v : out) {
    v = rng.next();
  }
  return out;
}

std::vector<std::string> strings(std::size_t n) {
  XorShift32 rng;
  std::vector<std::string> out(n);
  for (auto& s : out) {
    const std::uint32_t len = 8 + rng.next() % 9;
    s.resize(len);
    for (auto& ch : s) {
      ch = static_cast<char>('a' + rng.next() % 26);
    }
  }
  return out;
}

std::vector<std::uint8_t> sha_buffer(std::size_t size) {
  XorShift32 rng;
  std::vector<std::uint8_t> block;
  block.reserve(kShaBlockValues * 4);
  for (std::size_t i = 0; i < kShaBlockValues; ++i) {
    const std::uint32_t v = rng.next();
    for (int shift = 0; shift < 32; shift += 8) {
      block.push_back(static_cast<std::uint8_t>(v >> shift));
    }
  }
  std::vector<std::uint8_t> out;
  out.reserve(size);
  for (std::size_t copies = size / block.size(); copies > 0; --copies) {
    out.insert(out.end(), block.begin(), block.end());
  }
  return out;
}

std::string log_text(std::size_t lines) {
  XorShift32 rng;
  std::string text;
  for (std::size_t i = 0; i < lines; ++i) {
    const auto method = kMethods[rng.next() % 4];
    const std::uint32_t version = 1 + rng.next() % 3;
    const std::uint32_t user = rng.next() % 100000;
    const std::uint32_t status = kStatuses[rng.next() % 12];
    const std::uint32_t ms = rng.next() % 1000;
    std::format_to(std::back_inserter(text), "{} /api/v{}/users/{} {} {}ms\n", method, version,
                   user, status, ms);
  }
  return text;
}

std::pair<std::vector<double>, std::vector<double>> matrices(std::size_t n) {
  XorShift32 rng;
  std::vector<double> a(n * n);
  std::vector<double> b(n * n);
  for (auto& v : a) {
    v = rng.next() % 10;
  }
  for (auto& v : b) {
    v = rng.next() % 10;
  }
  return {std::move(a), std::move(b)};
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> hashmap_input(std::size_t n) {
  auto inserts = ints(n);
  std::vector<std::int64_t> lookups(n);
  for (std::size_t i = 0; i < n; ++i) {
    lookups[i] = inserts[i] ^ static_cast<std::int64_t>(i & 1);
  }
  return {std::move(inserts), std::move(lookups)};
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error(std::format("cannot read {}", path.string()));
  }
  std::ostringstream text;
  text << file.rdbuf();
  return std::move(text).str();
}

}  // namespace kernels
