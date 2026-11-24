// Race: trie-arena (cpp)
// Extracted from suite/cpp/src/arena.cpp — build and run via suite/run.py

// The -arena kernels (../SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with every
// node in one std::vector (push_back, the standard library's growth) and 32-bit index links, the
// same algorithms and node layouts as rust/src/arena.rs and go/arena.go. Each vector is destroyed
// inside the timed call, as Rust drops its Vec there.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "algo.hpp"
#include "bg.hpp"
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

std::uint32_t index_of(std::size_t i) { return static_cast<std::uint32_t>(i); }

// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
struct TrieNode {
  std::uint32_t count = 0;
  std::array<std::uint32_t, 26> children{};
};

std::string trie_arena(const Words& inserted, const Words& prefixes) {
  std::vector<TrieNode> nodes(1);
  for (const auto& w : inserted) {
    std::uint32_t node = 0;
    for (const std::uint8_t ch : w) {
      const std::size_t i = ch - 'a';
      if (nodes[node].children[i] == 0) {
        nodes[node].children[i] = index_of(nodes.size());
        nodes.emplace_back();
      }
      node = nodes[node].children[i];
      ++nodes[node].count;
    }
  }
  std::uint64_t total = 0;
  for (const auto& p : prefixes) {
    // A prefix has at least one letter, so node 0 after the walk means it is absent.
    std::uint32_t node = 0;
    for (const std::uint8_t ch : p) {
      node = nodes[node].children[ch - 'a'];
      if (node == 0) {
        break;
      }
    }
    if (node != 0) {
      total += nodes[node].count;
    }
  }
  return std::format("{}:{}", nodes.size() - 1, total);
}

Outcome trie_arena_kernel(const Args& args) {
  const auto [inserted, prefixes] = words(args.size);
  return outcome(measure(args.repeats, [&] { return trie_arena(inserted, prefixes); }));
}

}  // namespace kernels
