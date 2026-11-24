// Race: binary-trees (cpp)
// Extracted from suite/cpp/src/bg.cpp — build and run via suite/run.py

// Benchmarks Game kernels, single-threaded (../SPEC.md section 4). Floating-point code keeps the
// operation order of the spec so every language prints the same digits.
#include "bg.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <numbers>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

struct TreeNode {
  std::unique_ptr<TreeNode> left;
  std::unique_ptr<TreeNode> right;
};

std::unique_ptr<TreeNode> make_tree(int depth) {
  auto node = std::make_unique<TreeNode>();
  if (depth > 0) {
    node->left = make_tree(depth - 1);
    node->right = make_tree(depth - 1);
  }
  return node;
}

std::uint64_t check_tree(const TreeNode& node) {
  if (node.left && node.right) {
    return 1 + check_tree(*node.left) + check_tree(*node.right);
  }
  return 1;
}

Outcome binary_trees_kernel(const Args& args) {
  const int depth = static_cast<int>(args.size);
  auto [text, times] =
      measure(args.repeats, [depth] { return binary_trees_lines(depth, depth_checks); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

std::uint64_t depth_checks(int depth, std::uint64_t iterations) {
  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < iterations; ++i) {
    sum += check_tree(*make_tree(depth));
  }
  return sum;
}

std::string binary_trees_lines(int n,
                               const std::function<std::uint64_t(int, std::uint64_t)>& depth_run) {
  const int max_depth = std::max(kMinDepth + 2, n);
  const int stretch = max_depth + 1;
  std::string out;
  std::format_to(std::back_inserter(out), "stretch tree of depth {}\t check: {}\n", stretch,
                 check_tree(*make_tree(stretch)));
  const auto long_lived = make_tree(max_depth);
  for (int depth = kMinDepth; depth <= max_depth; depth += 2) {
    const std::uint64_t iterations = std::uint64_t{1} << (max_depth - depth + kMinDepth);
    std::format_to(std::back_inserter(out), "{}\t trees of depth {}\t check: {}\n", iterations,
                   depth, depth_run(depth, iterations));
  }
  std::format_to(std::back_inserter(out), "long lived tree of depth {}\t check: {}\n", max_depth,
                 check_tree(*long_lived));
  return out;
}

}  // namespace kernels
