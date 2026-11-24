// Race: fib (cpp)
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

std::uint64_t fib(std::uint32_t n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

Outcome fib_kernel(const Args& args) {
  const auto n = static_cast<std::uint32_t>(args.size);
  return outcome(measure(args.repeats, [n] { return std::to_string(fib(n)); }));
}

}  // namespace kernels
