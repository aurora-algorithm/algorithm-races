// Race: nqueens (cpp)
// Extracted from suite/cpp/src/algo_misc.cpp — build and run via suite/run.py

// edit-distance, knapsack, lis, kmp, str-find, fib, nqueens, binary-search, and the algo dispatch
// table
// (../SPEC.md section 12).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "algo.hpp"
#include "inputs.hpp"

namespace kernels {

struct Board {
  std::size_t n;
  std::vector<std::uint8_t> cols;
  std::vector<std::uint8_t> up;
  std::vector<std::uint8_t> down;

  std::uint64_t place(std::size_t row) {
    if (row == n) {
      return 1;
    }
    std::uint64_t count = 0;
    for (std::size_t col = 0; col < n; ++col) {
      const std::size_t u = row + col;
      const std::size_t d = row + n - 1 - col;
      if (!cols[col] && !up[u] && !down[d]) {
        cols[col] = up[u] = down[d] = 1;
        count += place(row + 1);
        cols[col] = up[u] = down[d] = 0;
      }
    }
    return count;
  }
};

std::uint64_t nqueens(std::size_t n) {
  Board board{n, std::vector<std::uint8_t>(n), std::vector<std::uint8_t>(2 * n - 1),
              std::vector<std::uint8_t>(2 * n - 1)};
  return board.place(0);
}

Outcome nqueens_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return std::to_string(nqueens(args.size)); }));
}

}  // namespace kernels
