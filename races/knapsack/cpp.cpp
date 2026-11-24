// Race: knapsack (cpp)
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

constexpr std::size_t kKnapsackItems = 1000;

struct Item {
  std::size_t weight;
  std::uint64_t value;
};

std::vector<Item> items() {
  XorShift32 rng;
  std::vector<Item> out(kKnapsackItems);
  for (Item& it : out) {
    it.weight = 1 + rng.next() % 1000;
    it.value = 1 + rng.next() % 1000;
  }
  return out;
}

std::vector<std::uint64_t> knapsack(std::size_t capacity, const std::vector<Item>& things) {
  std::vector<std::uint64_t> best(capacity + 1);
  for (const auto& [weight, value] : things) {
    for (std::size_t c = capacity; c >= weight; --c) {
      if (best[c - weight] + value > best[c]) {
        best[c] = best[c - weight] + value;
      }
    }
  }
  return best;
}

Outcome knapsack_kernel(const Args& args) {
  const auto things = items();
  auto [best, times] = measure(args.repeats, [&] { return knapsack(args.size, things); });
  return {std::format("{}:{}", best.back(), h31(best)), std::move(times), {}};
}

}  // namespace kernels
