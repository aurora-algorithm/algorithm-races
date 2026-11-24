// Race: collatz (cpp)
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

std::string collatz(std::size_t n) {
  std::vector<std::uint32_t> cache(n);
  cache[1] = 1;
  std::size_t best = 1;
  std::uint32_t best_len = 1;
  std::uint64_t total = 1;
  for (std::size_t start = 2; start < n; ++start) {
    std::uint64_t x = start;
    std::uint32_t steps = 0;
    while (x >= start) {
      x = x % 2 == 0 ? x / 2 : 3 * x + 1;
      ++steps;
    }
    const std::uint32_t len = steps + cache[x];
    cache[start] = len;
    total += len;
    if (len > best_len) {
      best = start;
      best_len = len;
    }
  }
  return std::format("{}:{}:{}:{}", n, best, best_len, total);
}

Outcome collatz_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return collatz(args.size); }));
}

}  // namespace kernels
