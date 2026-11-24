// Race: hyperloglog (cpp)
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

constexpr int kHllP = 14;

constexpr std::size_t kHllM = std::size_t{1} << kHllP;

struct HllResult {
  std::uint64_t estimate;
  std::vector<std::uint8_t> registers;
};

HllResult hyperloglog(std::size_t n) {
  XorShift32 rng;
  const auto universe = static_cast<std::uint32_t>(std::max<std::size_t>(n / 4, 1));
  std::vector<std::uint8_t> registers(kHllM);
  for (std::size_t i = 0; i < n; ++i) {
    const std::uint32_t h = fmix32(rng.next() % universe);
    const std::uint32_t w = h << kHllP;
    const int rank = w == 0 ? 32 - kHllP + 1 : std::countl_zero(w) + 1;
    auto& reg = registers[h >> (32 - kHllP)];
    reg = std::max(reg, static_cast<std::uint8_t>(rank));
  }
  double z = 0.0;
  std::size_t zeros = 0;
  for (const std::uint8_t r : registers) {
    z += std::pow(2.0, -static_cast<int>(r));
    zeros += r == 0 ? 1 : 0;
  }
  const double m = static_cast<double>(kHllM);
  double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
  if (estimate <= 2.5 * m && zeros > 0) {
    estimate = m * std::log(m / static_cast<double>(zeros));
  }
  return {static_cast<std::uint64_t>(std::floor(estimate + 0.5)), std::move(registers)};
}

Outcome hyperloglog_kernel(const Args& args) {
  auto [r, times] = measure(args.repeats, [&] { return hyperloglog(args.size); });
  return {std::format("{}:{}:{}", args.size, r.estimate, h31(r.registers)), std::move(times), {}};
}

}  // namespace kernels
