// Race: binary-search (cpp)
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

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> sorted_array(std::size_t n) {
  XorShift32 rng;
  std::vector<std::int64_t> a(n);
  std::int64_t v = 0;
  for (auto& x : a) {
    v += 1 + rng.next() % 4;
    x = v;
  }
  std::vector<std::int64_t> queries(n);
  for (auto& q : queries) {
    q = static_cast<std::int64_t>(rng.next()) % (v + 1);
  }
  return {std::move(a), std::move(queries)};
}

std::string binary_search(const std::vector<std::int64_t>& a,
                          const std::vector<std::int64_t>& queries) {
  const std::size_t n = a.size();
  std::uint64_t hits = 0;
  std::uint64_t total = 0;
  for (const std::int64_t q : queries) {
    std::size_t lo = 0;
    std::size_t hi = n;
    while (lo < hi) {
      const std::size_t mid = (lo + hi) / 2;
      if (a[mid] < q) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    if (lo < n && a[lo] == q) {
      ++hits;
    }
    total += lo;
  }
  return std::format("{}:{}", hits, total);
}

Outcome binary_search_kernel(const Args& args) {
  const auto [a, queries] = sorted_array(args.size);
  return outcome(measure(args.repeats, [&] { return binary_search(a, queries); }));
}

}  // namespace kernels
