// Race: bwt (cpp)
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

constexpr std::string_view kDna = "ACGT";

// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
std::vector<std::uint8_t> dna_from(XorShift32& rng, std::size_t n) {
  std::vector<std::uint8_t> out(n);
  for (std::size_t start = 0; start < n; start += 16) {
    std::uint32_t v = rng.next();
    for (std::size_t i = start; i < std::min(n, start + 16); ++i) {
      out[i] = static_cast<std::uint8_t>(kDna[v & 3]);
      v >>= 2;
    }
  }
  return out;
}

struct BwtResult {
  std::size_t n;
  int rounds;
  std::size_t primary;
  std::vector<std::uint8_t> out;
};

BwtResult bwt(const std::vector<std::uint8_t>& dna) {
  std::vector<std::uint8_t> text = dna;
  text.push_back('$');
  const std::size_t n = text.size();
  std::vector<std::int32_t> rank(text.begin(), text.end());
  std::vector<std::size_t> sa(n);
  std::iota(sa.begin(), sa.end(), std::size_t{0});
  std::vector<std::int32_t> second(n);
  std::vector<std::int32_t> tmp(n);
  std::size_t k = 1;
  int rounds = 0;
  while (true) {
    ++rounds;
    for (std::size_t i = 0; i < n; ++i) {
      second[i] = i + k < n ? rank[i + k] : -1;
    }
    std::ranges::sort(sa, [&](std::size_t a, std::size_t b) {
      return std::tie(rank[a], second[a]) < std::tie(rank[b], second[b]);
    });
    tmp[sa[0]] = 0;
    for (std::size_t j = 1; j < n; ++j) {
      const std::size_t a = sa[j - 1];
      const std::size_t b = sa[j];
      const bool differs = rank[a] != rank[b] || second[a] != second[b];
      tmp[b] = tmp[a] + (differs ? 1 : 0);
    }
    std::swap(rank, tmp);
    if (static_cast<std::size_t>(rank[sa[n - 1]]) == n - 1) {
      break;
    }
    k *= 2;
  }
  std::size_t primary = 0;
  std::vector<std::uint8_t> out(n);
  for (std::size_t j = 0; j < n; ++j) {
    if (sa[j] == 0) {
      primary = j;
      out[j] = text[n - 1];
    } else {
      out[j] = text[sa[j] - 1];
    }
  }
  return {n, rounds, primary, std::move(out)};
}

Outcome bwt_kernel(const Args& args) {
  XorShift32 rng;
  const auto dna = dna_from(rng, args.size);
  auto [r, times] = measure(args.repeats, [&] { return bwt(dna); });
  return {std::format("{}:{}:{}:{}", r.n, r.rounds, r.primary, fnv1a(r.out)), std::move(times), {}};
}

}  // namespace kernels
