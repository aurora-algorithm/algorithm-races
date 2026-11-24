// Race: palindrome (cpp)
// Extracted from suite/cpp/src/puzzle_text.cpp — build and run via suite/run.py

// The string kernels of group `puzzle` (../SPEC.md section 13): palindrome, bwt, aho-corasick,
// huffman.
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <numeric>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "puzzle.hpp"

namespace kernels {

// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
std::vector<std::uint8_t> binary_text(std::size_t n) {
  XorShift32 rng;
  std::vector<std::uint8_t> out(n);
  for (std::size_t start = 0; start < n; start += 32) {
    std::uint32_t v = rng.next();
    for (std::size_t i = start; i < std::min(n, start + 32); ++i) {
      out[i] = (v & 1) == 1 ? 'b' : 'a';
      v >>= 1;
    }
  }
  return out;
}

std::string palindrome(const std::vector<std::uint8_t>& text) {
  std::vector<std::uint8_t> t(2 * text.size() + 3, '#');
  const std::size_t last = t.size() - 1;
  t[0] = '^';
  t[last] = '$';
  for (std::size_t i = 0; i < text.size(); ++i) {
    t[2 + 2 * i] = text[i];
  }
  std::vector<std::size_t> p(t.size());
  std::size_t center = 0;
  std::size_t right = 0;
  for (std::size_t i = 1; i < last; ++i) {
    if (i < right) {
      p[i] = std::min(right - i, p[2 * center - i]);
    }
    while (t[i + 1 + p[i]] == t[i - 1 - p[i]]) {
      ++p[i];
    }
    if (i + p[i] > right) {
      center = i;
      right = i + p[i];
    }
  }
  std::size_t longest = 0;
  std::size_t best = 0;
  std::size_t count = 0;
  for (std::size_t i = 0; i < p.size(); ++i) {
    count += (p[i] + 1) / 2;
    if (p[i] > longest) {
      longest = p[i];
      best = i;
    }
  }
  return std::format("{}:{}:{}:{}", text.size(), longest, (best - 1 - longest) / 2, count);
}

Outcome palindrome_kernel(const Args& args) {
  const auto text = binary_text(args.size);
  return outcome(measure(args.repeats, [&] { return palindrome(text); }));
}

}  // namespace kernels
