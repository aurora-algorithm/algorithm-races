// Race: kmp (cpp)
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

constexpr std::size_t kPatternLen = 8;

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

struct Matches {
  std::size_t count = 0;
  std::int64_t first = -1;
  std::int64_t last = -1;

  void add(std::size_t pos) {
    const auto p = static_cast<std::int64_t>(pos);
    if (first < 0) {
      first = p;
    }
    last = p;
    ++count;
  }

  std::string str() const { return std::format("{}:{}:{}", count, first, last); }
};

std::pair<std::string, std::string> text(std::size_t n) {
  auto t = std::move(dna(n, 1)[0]);
  auto p = t.substr(n / 2, kPatternLen);
  return {std::move(t), std::move(p)};
}

Matches kmp(std::string_view t, std::string_view p) {
  const std::size_t m = p.size();
  std::vector<std::size_t> fail(m);
  std::size_t k = 0;
  for (std::size_t i = 1; i < m; ++i) {
    while (k > 0 && p[i] != p[k]) {
      k = fail[k - 1];
    }
    if (p[i] == p[k]) {
      ++k;
    }
    fail[i] = k;
  }
  Matches found;
  k = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    while (k > 0 && t[i] != p[k]) {
      k = fail[k - 1];
    }
    if (t[i] == p[k]) {
      ++k;
    }
    if (k == m) {
      found.add(i + 1 - m);
      k = fail[k - 1];
    }
  }
  return found;
}

Outcome kmp_kernel(const Args& args) {
  const auto [t, p] = text(args.size);
  auto [found, times] = measure(args.repeats, [&] { return kmp(t, p); });
  return {found.str(), std::move(times), {}};
}

}  // namespace kernels
