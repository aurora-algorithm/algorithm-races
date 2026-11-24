// Race: cycle (cpp)
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

constexpr std::uint64_t kCycleMod = 67108859;

constexpr std::uint64_t kCycleC = 12345;

std::uint64_t step(std::uint64_t x) { return (x * x + kCycleC) % kCycleMod; }

std::string cycle(const std::vector<std::uint64_t>& starts) {
  std::uint64_t total_mu = 0;
  std::uint64_t total_lam = 0;
  std::uint64_t longest = 0;
  for (const std::uint64_t x0 : starts) {
    std::uint64_t tortoise = step(x0);
    std::uint64_t hare = step(step(x0));
    while (tortoise != hare) {
      tortoise = step(tortoise);
      hare = step(step(hare));
    }
    std::uint64_t mu = 0;
    tortoise = x0;
    while (tortoise != hare) {
      tortoise = step(tortoise);
      hare = step(hare);
      ++mu;
    }
    std::uint64_t lam = 1;
    hare = step(tortoise);
    while (tortoise != hare) {
      hare = step(hare);
      ++lam;
    }
    total_mu += mu;
    total_lam += lam;
    longest = std::max(longest, mu + lam);
  }
  return std::format("{}:{}:{}:{}", starts.size(), total_mu, total_lam, longest);
}

Outcome cycle_kernel(const Args& args) {
  XorShift32 rng;
  std::vector<std::uint64_t> starts(args.size);
  for (auto& s : starts) {
    s = rng.next() % kCycleMod;
  }
  return outcome(measure(args.repeats, [&] { return cycle(starts); }));
}

}  // namespace kernels
