// Race: aho-corasick (cpp)
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

constexpr std::size_t kOneMib = std::size_t{1} << 20;

constexpr std::string_view kDna = "ACGT";

std::vector<std::uint8_t> repeat(const std::vector<std::uint8_t>& block, std::size_t copies) {
  std::vector<std::uint8_t> out;
  out.reserve(block.size() * copies);
  for (std::size_t i = 0; i < copies; ++i) {
    out.insert(out.end(), block.begin(), block.end());
  }
  return out;
}

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

constexpr std::size_t kAhoPatterns = 10000;

struct AhoInput {
  std::vector<std::vector<std::uint8_t>> patterns;
  std::vector<std::uint8_t> text;
};

AhoInput aho_input(std::size_t n) {
  XorShift32 rng;
  std::vector<std::vector<std::uint8_t>> patterns(kAhoPatterns);
  for (auto& pattern : patterns) {
    pattern.resize(6 + rng.next() % 7);
    for (auto& ch : pattern) {
      ch = static_cast<std::uint8_t>(kDna[rng.next() % 4]);
    }
  }
  const auto block = dna_from(rng, kOneMib);
  return {std::move(patterns), repeat(block, n / kOneMib)};
}

std::string aho_corasick(const AhoInput& in) {
  std::array<std::size_t, 256> code{};
  for (std::size_t i = 0; i < kDna.size(); ++i) {
    code[static_cast<std::uint8_t>(kDna[i])] = i;
  }
  std::vector<std::int32_t> go(4, -1);
  std::vector<std::uint64_t> out(1);
  for (const auto& pattern : in.patterns) {
    std::size_t s = 0;
    for (const std::uint8_t ch : pattern) {
      const std::size_t edge = s * 4 + code[ch];
      if (go[edge] == -1) {
        go[edge] = static_cast<std::int32_t>(out.size());
        go.insert(go.end(), 4, -1);
        out.push_back(0);
      }
      s = static_cast<std::size_t>(go[edge]);
    }
    ++out[s];
  }
  std::vector<std::size_t> fail(out.size());
  std::vector<std::size_t> queue;
  queue.reserve(out.size());
  for (std::size_t c = 0; c < 4; ++c) {
    if (go[c] == -1) {
      go[c] = 0;
    } else {
      queue.push_back(static_cast<std::size_t>(go[c]));
    }
  }
  for (std::size_t head = 0; head < queue.size(); ++head) {
    const std::size_t s = queue[head];
    out[s] += out[fail[s]];
    for (std::size_t c = 0; c < 4; ++c) {
      const std::int32_t fallback = go[fail[s] * 4 + c];
      const std::int32_t t = go[s * 4 + c];
      if (t == -1) {
        go[s * 4 + c] = fallback;
      } else {
        fail[static_cast<std::size_t>(t)] = static_cast<std::size_t>(fallback);
        queue.push_back(static_cast<std::size_t>(t));
      }
    }
  }
  std::size_t s = 0;
  std::uint64_t matches = 0;
  std::uint64_t hits = 0;
  for (const std::uint8_t ch : in.text) {
    s = static_cast<std::size_t>(go[s * 4 + code[ch]]);
    if (out[s] > 0) {
      matches += out[s];
      ++hits;
    }
  }
  return std::format("{}:{}:{}:{}", in.patterns.size(), out.size(), matches, hits);
}

Outcome aho_corasick_kernel(const Args& args) {
  const auto input = aho_input(args.size);
  return outcome(measure(args.repeats, [&] { return aho_corasick(input); }));
}

}  // namespace kernels
