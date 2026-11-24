// Race: lis (cpp)
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

std::vector<std::int64_t> lis(const std::vector<std::int64_t>& values) {
  std::vector<std::int64_t> tails;
  for (const std::int64_t x : values) {
    const auto it = std::ranges::lower_bound(tails, x);
    if (it == tails.end()) {
      tails.push_back(x);
    } else {
      *it = x;
    }
  }
  return tails;
}

Outcome lis_kernel(const Args& args) {
  const auto values = ints(args.size);
  auto [tails, times] = measure(args.repeats, [&] { return lis(values); });
  return {std::format("{}:{}", tails.size(), h31(tails)), std::move(times), {}};
}

}  // namespace kernels
