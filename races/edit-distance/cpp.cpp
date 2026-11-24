// Race: edit-distance (cpp)
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

constexpr std::string_view kAcgt = "acgt";

std::vector<std::string> dna(std::size_t length, std::size_t count) {
  XorShift32 rng;
  std::vector<std::string> out(count);
  for (auto& s : out) {
    s.resize(length);
    for (auto& ch : s) {
      ch = kAcgt[rng.next() % 4];
    }
  }
  return out;
}

std::string edit_distance(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min({prev[j + 1] + 1, cur[j] + 1, prev[j] + cost});
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

// edit_distance with two nested two-argument std::min calls in place of the initializer list
// (runtime variants cpp-*-edmin2, KERNELS_ED_MIN=min2).
std::string edit_distance_min2(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min(std::min(prev[j + 1] + 1, cur[j] + 1), prev[j] + cost);
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

// edit_distance_min2 with the left neighbour (cur[j]) compared last, the order of the C variant
// c-clang-edleft (runtime variants cpp-*-edmin2l, KERNELS_ED_MIN=min2l).
std::string edit_distance_min2l(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min(std::min(prev[j + 1] + 1, prev[j] + cost), cur[j] + 1);
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

bool env_is(const char* name, std::string_view value) {
  const char* got =
      std::getenv(name);  // NOLINT(concurrency-mt-unsafe): read once, before any thread
  return got != nullptr && value == got;
}

Outcome edit_distance_kernel(const Args& args) {
  const auto ab = dna(args.size, 2);
  if (env_is("KERNELS_ED_MIN", "min2")) {
    return outcome(measure(args.repeats, [&] { return edit_distance_min2(ab[0], ab[1]); }));
  }
  if (env_is("KERNELS_ED_MIN", "min2l")) {
    return outcome(measure(args.repeats, [&] { return edit_distance_min2l(ab[0], ab[1]); }));
  }
  return outcome(measure(args.repeats, [&] { return edit_distance(ab[0], ab[1]); }));
}

}  // namespace kernels
