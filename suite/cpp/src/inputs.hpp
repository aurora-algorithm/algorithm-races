// Shared inputs and checksums (../SPEC.md section 2).
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kernels {

// Marsaglia's xorshift32; every input starts from a fresh generator.
class XorShift32 {
 public:
  static constexpr std::uint32_t kSeed = 2463534242u;

  explicit XorShift32(std::uint32_t seed = kSeed) : state_(seed) {}

  std::uint32_t next() {
    std::uint32_t s = state_;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    state_ = s;
    return s;
  }

 private:
  std::uint32_t state_;
};

// `h = h * 31 + v` modulo 2^32; only the low 32 bits of each value matter.
template <class Range>
std::uint32_t h31(const Range& values, std::uint32_t h = 0) {
  for (const auto v : values) {
    h = h * 31u + static_cast<std::uint32_t>(v);
  }
  return h;
}

std::uint32_t fnv1a(std::span<const std::uint8_t> bytes);
std::uint32_t fnv1a(std::string_view text);

std::vector<std::int64_t> ints(std::size_t n);
std::vector<std::string> strings(std::size_t n);
std::vector<std::uint8_t> sha_buffer(std::size_t size);
std::string log_text(std::size_t lines);
std::pair<std::vector<double>, std::vector<double>> matrices(std::size_t n);
// Keys to insert, and keys to look up: even i is a key in the map, odd i its neighbour.
std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> hashmap_input(std::size_t n);

// Reads a whole file; throws std::runtime_error naming the path when it cannot.
std::string read_file(const std::filesystem::path& path);

}  // namespace kernels
