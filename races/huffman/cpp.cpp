// Race: huffman (cpp)
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

std::vector<std::uint8_t> repeat(const std::vector<std::uint8_t>& block, std::size_t copies) {
  std::vector<std::uint8_t> out;
  out.reserve(block.size() * copies);
  for (std::size_t i = 0; i < copies; ++i) {
    out.insert(out.end(), block.begin(), block.end());
  }
  return out;
}

// A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
std::vector<std::uint8_t> skewed_text(std::size_t n) {
  XorShift32 rng;
  std::vector<std::uint8_t> block(kOneMib);
  for (auto& ch : block) {
    const std::uint32_t a = rng.next() % 26;
    const std::uint32_t b = rng.next() % 26;
    ch = static_cast<std::uint8_t>(97 + a * b / 26);
  }
  return repeat(block, n / kOneMib);
}

// Children of the internal nodes 256, 257, ...; ids below 256 are leaves (the byte itself).
struct Tree {
  std::array<std::size_t, 512> left{};
  std::array<std::size_t, 512> right{};
  std::size_t root = 0;
};

Tree huffman_tree(const std::array<std::uint64_t, 256>& freq) {
  std::priority_queue<std::uint64_t, std::vector<std::uint64_t>, std::greater<>> heap;
  for (std::uint64_t sym = 0; sym < 256; ++sym) {
    if (freq[sym] > 0) {
      heap.push(freq[sym] * 1024 + sym);
    }
  }
  Tree tree;
  std::size_t next_id = 256;
  while (heap.size() > 1) {
    const std::uint64_t a = heap.top();
    heap.pop();
    const std::uint64_t b = heap.top();
    heap.pop();
    tree.left[next_id] = static_cast<std::size_t>(a % 1024);
    tree.right[next_id] = static_cast<std::size_t>(b % 1024);
    heap.push((a / 1024 + b / 1024) * 1024 + next_id);
    ++next_id;
  }
  tree.root = heap.empty() ? 0 : static_cast<std::size_t>(heap.top() % 1024);
  return tree;
}

struct Codes {
  std::array<std::uint64_t, 256> code{};
  std::array<std::uint32_t, 256> length{};
};

Codes huffman_codes(const Tree& tree) {
  Codes codes;
  std::vector<std::tuple<std::size_t, std::uint64_t, std::uint32_t>> stack = {{tree.root, 0, 0}};
  while (!stack.empty()) {
    const auto [node, code, len] = stack.back();
    stack.pop_back();
    if (node < 256) {
      codes.code[node] = code;
      codes.length[node] = len;
    } else {
      stack.emplace_back(tree.left[node], code << 1, len + 1);
      stack.emplace_back(tree.right[node], (code << 1) | 1, len + 1);
    }
  }
  return codes;
}

std::pair<std::uint64_t, std::vector<std::uint8_t>> huffman_encode(
    const std::vector<std::uint8_t>& text, const Codes& codes) {
  std::vector<std::uint8_t> encoded;
  std::uint64_t acc = 0;
  std::uint32_t nbits = 0;
  std::uint64_t bits = 0;
  for (const std::uint8_t b : text) {
    const std::uint32_t len = codes.length[b];
    acc = (acc << len) | codes.code[b];
    nbits += len;
    bits += len;
    while (nbits >= 8) {
      nbits -= 8;
      encoded.push_back(static_cast<std::uint8_t>(acc >> nbits));
    }
    acc &= (std::uint64_t{1} << nbits) - 1;
  }
  if (nbits > 0) {
    encoded.push_back(static_cast<std::uint8_t>(acc << (8 - nbits)));
  }
  return {bits, std::move(encoded)};
}

std::vector<std::uint8_t> huffman_decode(const Tree& tree, const std::vector<std::uint8_t>& encoded,
                                         std::size_t n) {
  std::vector<std::uint8_t> decoded;
  decoded.reserve(n);
  std::size_t node = tree.root;
  for (const std::uint8_t byte : encoded) {
    for (int shift = 7; shift >= 0; --shift) {
      node = ((byte >> shift) & 1) == 1 ? tree.right[node] : tree.left[node];
      if (node < 256) {
        decoded.push_back(static_cast<std::uint8_t>(node));
        node = tree.root;
        if (decoded.size() == n) {
          return decoded;
        }
      }
    }
  }
  return decoded;
}

struct HuffmanResult {
  std::uint64_t bits;
  std::vector<std::uint8_t> encoded;
  std::vector<std::uint8_t> decoded;
};

HuffmanResult huffman(const std::vector<std::uint8_t>& text) {
  std::array<std::uint64_t, 256> freq{};
  for (const std::uint8_t b : text) {
    ++freq[b];
  }
  const Tree tree = huffman_tree(freq);
  const Codes codes = huffman_codes(tree);
  auto [bits, encoded] = huffman_encode(text, codes);
  auto decoded = huffman_decode(tree, encoded, text.size());
  return {bits, std::move(encoded), std::move(decoded)};
}

Outcome huffman_kernel(const Args& args) {
  const auto text = skewed_text(args.size);
  auto [r, times] = measure(args.repeats, [&] { return huffman(text); });
  return {
      std::format("{}:{}:{}:{}", text.size(), r.bits, fnv1a(r.encoded), r.decoded == text ? 1 : 0),
      std::move(times),
      {}};
}

}  // namespace kernels
