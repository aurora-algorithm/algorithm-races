// Race: window-max (cpp)
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

constexpr std::array<std::size_t, 5> kWindows = {10, 100, 1000, 10000, 100000};

std::string window_max(const std::vector<std::int64_t>& values) {
  const std::size_t n = values.size();
  std::vector<std::size_t> dq(n);
  std::string out = std::to_string(n);
  for (const std::size_t w : kWindows) {
    std::size_t head = 0;
    std::size_t tail = 0;
    std::uint32_t h = 0;
    for (std::size_t i = 0; i < n; ++i) {
      const std::int64_t v = values[i];
      while (tail > head && values[dq[tail - 1]] <= v) {
        --tail;
      }
      dq[tail++] = i;
      if (dq[head] + w <= i) {
        ++head;
      }
      if (i + 1 >= w) {
        h = h * 31u + static_cast<std::uint32_t>(values[dq[head]]);
      }
    }
    out += std::format(":{}", h);
  }
  return out;
}

Outcome window_max_kernel(const Args& args) {
  const auto values = ints(args.size);
  return outcome(measure(args.repeats, [&] { return window_max(values); }));
}

}  // namespace kernels
