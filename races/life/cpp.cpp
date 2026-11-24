// Race: life (cpp)
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

constexpr std::size_t kLifeSide = 512;

std::vector<std::uint8_t> life(const std::vector<std::uint8_t>& grid, std::size_t generations) {
  constexpr std::size_t w = kLifeSide;
  std::vector<std::uint8_t> cur = grid;
  std::vector<std::uint8_t> next(grid.size());
  for (std::size_t g = 0; g < generations; ++g) {
    for (std::size_t y = 0; y < w; ++y) {
      const std::size_t up = (y + w - 1) % w * w;
      const std::size_t row = y * w;
      const std::size_t down = (y + 1) % w * w;
      for (std::size_t x = 0; x < w; ++x) {
        const std::size_t left = (x + w - 1) % w;
        const std::size_t right = (x + 1) % w;
        const int n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] +
                      cur[row + right] + cur[down + left] + cur[down + x] + cur[down + right];
        next[row + x] = (n == 3 || (n == 2 && cur[row + x] == 1)) ? 1 : 0;
      }
    }
    std::swap(cur, next);
  }
  return cur;
}

Outcome life_kernel(const Args& args) {
  XorShift32 rng;
  std::vector<std::uint8_t> grid(kLifeSide * kLifeSide);
  for (auto& cell : grid) {
    cell = rng.next() % 4 == 0 ? 1 : 0;
  }
  auto [cells, times] = measure(args.repeats, [&] { return life(grid, args.size); });
  std::size_t alive = 0;
  for (const std::uint8_t c : cells) {
    alive += c;
  }
  return {std::format("{}:{}:{}", args.size, alive, fnv1a(cells)), std::move(times), {}};
}

}  // namespace kernels
