// Race: rain-water (cpp)
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

std::string rain_water(std::size_t side, const std::vector<std::uint64_t>& heights) {
  const std::size_t n = heights.size();
  std::vector<std::uint8_t> seen(n);
  std::priority_queue<std::uint64_t, std::vector<std::uint64_t>, std::greater<>> heap;
  for (std::size_t idx = 0; idx < n; ++idx) {
    const std::size_t r = idx / side;
    const std::size_t c = idx % side;
    if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
      seen[idx] = 1;
      heap.push(heights[idx] * n + idx);
    }
  }
  std::uint64_t water = 0;
  std::uint64_t wet = 0;
  while (!heap.empty()) {
    const std::uint64_t key = heap.top();
    heap.pop();
    const std::uint64_t level = key / n;
    const auto idx = static_cast<std::size_t>(key % n);
    const std::size_t r = idx / side;
    const std::size_t c = idx % side;
    const std::pair<std::size_t, bool> neighbours[] = {
        {idx - side, r > 0},
        {idx + side, r + 1 < side},
        {idx - 1, c > 0},
        {idx + 1, c + 1 < side},
    };
    for (const auto& [next, ok] : neighbours) {
      if (!ok || seen[next]) {
        continue;
      }
      seen[next] = 1;
      std::uint64_t h = heights[next];
      if (h < level) {
        water += level - h;
        ++wet;
        h = level;
      }
      heap.push(h * n + next);
    }
  }
  return std::format("{}:{}:{}", side, water, wet);
}

Outcome rain_water_kernel(const Args& args) {
  const std::size_t side = args.size;
  XorShift32 rng;
  std::vector<std::uint64_t> heights(side * side);
  for (auto& h : heights) {
    h = rng.next() % 1000;
  }
  return outcome(measure(args.repeats, [&] { return rain_water(side, heights); }));
}

}  // namespace kernels
