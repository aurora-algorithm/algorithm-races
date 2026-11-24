// Race: bst (cpp)
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

struct BstNode {
  std::int64_t key;
  std::uint32_t count = 1;
  std::unique_ptr<BstNode> left;
  std::unique_ptr<BstNode> right;
};

class Bst {
 public:
  void insert(std::int64_t key) {
    std::unique_ptr<BstNode>* slot = &root_;
    while (*slot) {
      BstNode& node = **slot;
      if (key < node.key) {
        slot = &node.left;
      } else if (key > node.key) {
        slot = &node.right;
      } else {
        ++node.count;
        return;
      }
    }
    *slot = std::make_unique<BstNode>(key);
    ++size_;
  }

  bool contains(std::int64_t key) const {
    const BstNode* node = root_.get();
    while (node != nullptr) {
      if (key < node->key) {
        node = node->left.get();
      } else if (key > node->key) {
        node = node->right.get();
      } else {
        return true;
      }
    }
    return false;
  }

  // In-order walk with an explicit stack.
  std::vector<std::int64_t> keys() const {
    std::vector<std::int64_t> out;
    out.reserve(size_);
    std::vector<const BstNode*> stack;
    const BstNode* node = root_.get();
    while (node != nullptr || !stack.empty()) {
      while (node != nullptr) {
        stack.push_back(node);
        node = node->left.get();
      }
      node = stack.back();
      stack.pop_back();
      out.push_back(node->key);
      node = node->right.get();
    }
    return out;
  }

  std::size_t size() const { return size_; }

 private:
  std::unique_ptr<BstNode> root_;
  std::size_t size_ = 0;
};

// distinct keys, hits, the keys in order (hashed after the timer)
struct TreeResult {
  std::size_t size;
  std::size_t hits;
  std::vector<std::int64_t> keys;

  std::string checksum() const { return std::format("{}:{}:{}", size, hits, h31(keys)); }
};

TreeResult bst(const std::vector<std::int64_t>& inserts, const std::vector<std::int64_t>& lookups) {
  Bst tree;
  for (const std::int64_t k : inserts) {
    tree.insert(k);
  }
  std::size_t hits = 0;
  for (const std::int64_t k : lookups) {
    hits += tree.contains(k) ? 1 : 0;
  }
  return {tree.size(), hits, tree.keys()};
}

Outcome bst_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return bst(inserts, lookups); });
  return {r.checksum(), std::move(times), {}};
}

}  // namespace kernels
