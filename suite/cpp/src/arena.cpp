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
namespace {

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

// A binary tree's nodes, children pushed before their parent; a leaf's links are kNil.
struct TreeNode {
  std::uint32_t left;
  std::uint32_t right;
};

std::uint32_t make_tree(std::vector<TreeNode>& nodes, int depth) {
  TreeNode node{kNil, kNil};
  if (depth > 0) {
    node.left = make_tree(nodes, depth - 1);
    node.right = make_tree(nodes, depth - 1);
  }
  nodes.push_back(node);
  return index_of(nodes.size() - 1);
}

std::uint64_t check_tree(const std::vector<TreeNode>& nodes, std::uint32_t index) {
  const TreeNode node = nodes[index];
  if (node.left == kNil) {
    return 1;
  }
  return 1 + check_tree(nodes, node.left) + check_tree(nodes, node.right);
}

std::uint64_t tree_check(int depth) {
  std::vector<TreeNode> nodes;
  const std::uint32_t root = make_tree(nodes, depth);
  return check_tree(nodes, root);
}

std::string binary_trees_arena(int n) {
  const int max_depth = std::max(kMinDepth + 2, n);
  const int stretch = max_depth + 1;
  std::string out;
  std::format_to(std::back_inserter(out), "stretch tree of depth {}\t check: {}\n", stretch,
                 tree_check(stretch));
  std::vector<TreeNode> long_lived;
  const std::uint32_t long_root = make_tree(long_lived, max_depth);
  for (int depth = kMinDepth; depth <= max_depth; depth += 2) {
    const std::uint64_t iterations = std::uint64_t{1} << (max_depth - depth + kMinDepth);
    std::uint64_t check = 0;
    for (std::uint64_t i = 0; i < iterations; ++i) {
      check += tree_check(depth);
    }
    std::format_to(std::back_inserter(out), "{}\t trees of depth {}\t check: {}\n", iterations,
                   depth, check);
  }
  std::format_to(std::back_inserter(out), "long lived tree of depth {}\t check: {}\n", max_depth,
                 check_tree(long_lived, long_root));
  return out;
}

Outcome bst_arena_kernel(const Args& args) {
  if (args.size >= kNil) {
    throw std::invalid_argument(std::format("--size {} does not fit 32-bit links", args.size));
  }
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return bst_arena(inserts, lookups); });
  return {std::format("{}:{}:{}", r.size, r.hits, h31(r.keys)), std::move(times), {}};
}

Outcome trie_arena_kernel(const Args& args) {
  const auto [inserted, prefixes] = words(args.size);
  return outcome(measure(args.repeats, [&] { return trie_arena(inserted, prefixes); }));
}

Outcome binary_trees_arena_kernel(const Args& args) {
  // 2^(depth + 2) nodes in the stretch tree must stay below the kNil link.
  if (args.size > 29) {
    throw std::invalid_argument(std::format("--size {} is too deep for 32-bit links", args.size));
  }
  const int depth = static_cast<int>(args.size);
  auto [text, times] = measure(args.repeats, [depth] { return binary_trees_arena(depth); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

constexpr std::array kArenaKernels{
    Kernel{"bst-arena", bst_arena_kernel},
    Kernel{"trie-arena", trie_arena_kernel},
    Kernel{"binary-trees-arena", binary_trees_arena_kernel},
};

}  // namespace

std::span<const Kernel> arena_kernels() { return kArenaKernels; }

}  // namespace kernels
