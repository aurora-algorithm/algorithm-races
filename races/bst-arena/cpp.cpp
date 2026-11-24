// Race: bst-arena (cpp)
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

// The "no child" link of the BST and the binary trees.
constexpr std::uint32_t kNil = std::numeric_limits<std::uint32_t>::max();

struct BstNode {
  std::int64_t key;
  std::uint32_t count;
  std::uint32_t left;
  std::uint32_t right;
};

std::uint32_t index_of(std::size_t i) { return static_cast<std::uint32_t>(i); }

void bst_insert(std::vector<BstNode>& nodes, std::int64_t key) {
  const BstNode fresh{key, 1, kNil, kNil};
  if (nodes.empty()) {
    nodes.push_back(fresh);
    return;
  }
  const std::uint32_t next = index_of(nodes.size());
  std::uint32_t cur = 0;
  while (true) {
    BstNode& node = nodes[cur];
    if (key == node.key) {
      ++node.count;
      return;
    }
    std::uint32_t& link = key < node.key ? node.left : node.right;
    if (link == kNil) {
      link = next;
      nodes.push_back(fresh);
      return;
    }
    cur = link;
  }
}

bool bst_contains(const std::vector<BstNode>& nodes, std::int64_t key) {
  std::uint32_t cur = nodes.empty() ? kNil : 0;
  while (cur != kNil && nodes[cur].key != key) {
    const BstNode& node = nodes[cur];
    cur = key < node.key ? node.left : node.right;
  }
  return cur != kNil;
}

// In-order walk with an explicit stack of indexes.
std::vector<std::int64_t> in_order(const std::vector<BstNode>& nodes) {
  std::vector<std::int64_t> keys;
  keys.reserve(nodes.size());
  std::vector<std::uint32_t> stack;
  std::uint32_t cur = nodes.empty() ? kNil : 0;
  while (cur != kNil || !stack.empty()) {
    while (cur != kNil) {
      stack.push_back(cur);
      cur = nodes[cur].left;
    }
    const std::uint32_t top = stack.back();
    stack.pop_back();
    keys.push_back(nodes[top].key);
    cur = nodes[top].right;
  }
  return keys;
}

// distinct keys, hits, the keys in order (hashed after the timer)
struct BstResult {
  std::size_t size;
  std::size_t hits;
  std::vector<std::int64_t> keys;
};

BstResult bst_arena(const std::vector<std::int64_t>& inserts,
                    const std::vector<std::int64_t>& lookups) {
  std::vector<BstNode> nodes;
  for (const std::int64_t k : inserts) {
    bst_insert(nodes, k);
  }
  std::size_t hits = 0;
  for (const std::int64_t k : lookups) {
    hits += bst_contains(nodes, k) ? 1 : 0;
  }
  return {nodes.size(), hits, in_order(nodes)};
}

Outcome bst_arena_kernel(const Args& args) {
  if (args.size >= kNil) {
    throw std::invalid_argument(std::format("--size {} does not fit 32-bit links", args.size));
  }
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return bst_arena(inserts, lookups); });
  return {std::format("{}:{}:{}", r.size, r.hits, h31(r.keys)), std::move(times), {}};
}

}  // namespace kernels
