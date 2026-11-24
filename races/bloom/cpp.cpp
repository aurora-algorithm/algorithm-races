// Race: bloom (cpp)
// Extracted from suite/cpp/src/puzzle.cpp — build and run via suite/run.py

// bloom, hyperloglog, skiplist, cycle, life, window-max, rain-water, collatz, and the puzzle
// dispatch table
// (../SPEC.md section 13).
#include "puzzle.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace kernels {

// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
std::uint32_t fmix32(std::uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6bu;
  h ^= h >> 13;
  h *= 0xc2b2ae35u;
  h ^= h >> 16;
  return h;
}

constexpr std::size_t kBloomBitsPerKey = 10;

constexpr std::uint32_t kBloomHashes = 7;

constexpr std::uint32_t kGolden = 0x9e3779b9u;

std::string bloom(std::span<const std::int64_t> keys, std::span<const std::int64_t> lookups) {
  const std::size_t m = kBloomBitsPerKey * keys.size();
  std::vector<std::uint8_t> bits((m + 7) / 8);
  const auto m32 = static_cast<std::uint32_t>(m);
  // The bit positions of a key: h1 + i*h2 (Kirsch-Mitzenmacher), wrapping at 2^32, modulo m.
  const auto position = [m32](std::int64_t key, std::uint32_t i) -> std::size_t {
    const auto k = static_cast<std::uint32_t>(key);
    const std::uint32_t h1 = fmix32(k);
    const std::uint32_t h2 = fmix32(k ^ kGolden) | 1;
    return (h1 + i * h2) % m32;
  };
  for (const std::int64_t key : keys) {
    for (std::uint32_t i = 0; i < kBloomHashes; ++i) {
      const std::size_t b = position(key, i);
      bits[b >> 3] |= static_cast<std::uint8_t>(1 << (b & 7));
    }
  }
  std::size_t false_positives = 0;
  for (const std::int64_t key : lookups) {
    bool all = true;
    for (std::uint32_t i = 0; i < kBloomHashes && all; ++i) {
      const std::size_t b = position(key, i);
      all = (bits[b >> 3] & (1 << (b & 7))) != 0;
    }
    false_positives += all ? 1 : 0;
  }
  std::uint64_t set_bits = 0;
  for (const std::uint8_t byte : bits) {
    set_bits += static_cast<std::uint64_t>(std::popcount(byte));
  }
  return std::format("{}:{}:{}", keys.size(), set_bits, false_positives);
}

Outcome bloom_kernel(const Args& args) {
  const auto values = ints(2 * args.size);
  const std::span all(values);
  return outcome(
      measure(args.repeats, [&] { return bloom(all.first(args.size), all.subspan(args.size)); }));
}

}  // namespace kernels
