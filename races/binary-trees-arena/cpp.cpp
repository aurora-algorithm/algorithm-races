// Race: binary-trees-arena (cpp)
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

std::uint32_t index_of(std::size_t i) { return static_cast<std::uint32_t>(i); }

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

Outcome binary_trees_arena_kernel(const Args& args) {
  // 2^(depth + 2) nodes in the stretch tree must stay below the kNil link.
  if (args.size > 29) {
    throw std::invalid_argument(std::format("--size {} is too deep for 32-bit links", args.size));
  }
  const int depth = static_cast<int>(args.size);
  auto [text, times] = measure(args.repeats, [depth] { return binary_trees_arena(depth); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

}  // namespace kernels
