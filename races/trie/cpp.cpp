// Race: trie (cpp)
// Extracted from suite/cpp/src/algo_tree.cpp — build and run via suite/run.py

// bst, ordered-map, trie, lru (../SPEC.md section 12).
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "algo.hpp"
#include "inputs.hpp"

namespace kernels {

std::vector<std::uint8_t> word(XorShift32& rng, std::uint32_t min_len, std::uint32_t spread) {
  std::vector<std::uint8_t> w(min_len + rng.next() % spread);
  for (auto& ch : w) {
    ch = static_cast<std::uint8_t>('a' + rng.next() % 26);
  }
  return w;
}

struct TrieNode {
  std::uint64_t count = 0;
  std::array<std::unique_ptr<TrieNode>, 26> children;
};

std::string trie(const Words& inserted, const Words& prefixes) {
  TrieNode root;
  std::uint64_t nodes = 0;
  for (const auto& w : inserted) {
    TrieNode* node = &root;
    for (const std::uint8_t ch : w) {
      auto& child = node->children[ch - 'a'];
      if (!child) {
        child = std::make_unique<TrieNode>();
        ++nodes;
      }
      node = child.get();
      ++node->count;
    }
  }
  std::uint64_t total = 0;
  for (const auto& p : prefixes) {
    const TrieNode* node = &root;
    for (const std::uint8_t ch : p) {
      node = node->children[ch - 'a'].get();
      if (node == nullptr) {
        break;
      }
    }
    total += node != nullptr ? node->count : 0;
  }
  return std::format("{}:{}", nodes, total);
}

std::pair<Words, Words> words(std::size_t n) {
  XorShift32 rng;
  Words inserted(n);
  for (auto& w : inserted) {
    w = word(rng, 3, 8);
  }
  Words prefixes(n);
  for (auto& p : prefixes) {
    p = word(rng, 1, 4);
  }
  return {std::move(inserted), std::move(prefixes)};
}

Outcome trie_kernel(const Args& args) {
  const auto [inserted, prefixes] = words(args.size);
  return outcome(measure(args.repeats, [&] { return trie(inserted, prefixes); }));
}

}  // namespace kernels
