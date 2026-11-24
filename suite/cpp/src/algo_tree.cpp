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
namespace {

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

TreeResult ordered_map(const std::vector<std::int64_t>& inserts,
                       const std::vector<std::int64_t>& lookups) {
  std::map<std::int64_t, std::int64_t> counts;
  for (const std::int64_t k : inserts) {
    ++counts[k];
  }
  std::size_t hits = 0;
  for (const std::int64_t k : lookups) {
    hits += counts.contains(k) ? 1 : 0;
  }
  std::vector<std::int64_t> keys;
  keys.reserve(counts.size());
  for (const auto& entry : counts) {
    keys.push_back(entry.first);
  }
  return {counts.size(), hits, std::move(keys)};
}

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

struct LruInput {
  std::size_t capacity;
  std::vector<std::uint32_t> keys;
};

LruInput lru_keys(std::size_t n) {
  const std::size_t capacity = std::max<std::size_t>(n / 100, 1);
  XorShift32 rng;
  std::vector<std::uint32_t> keys(n);
  for (auto& k : keys) {
    k = rng.next() % static_cast<std::uint32_t>(2 * capacity);
  }
  return {capacity, std::move(keys)};
}

// The usual C++ LRU: a std::list in recency order (most recent first) and an unordered_map from key
// to its node.
struct LruResult {
  std::size_t hits;
  std::size_t misses;
  std::vector<std::uint32_t> oldest_first;
};

LruResult lru(const LruInput& in) {
  std::list<std::uint32_t> order;
  std::unordered_map<std::uint32_t, std::list<std::uint32_t>::iterator> nodes;
  nodes.reserve(in.capacity);
  std::size_t hits = 0;
  for (const std::uint32_t k : in.keys) {
    if (const auto it = nodes.find(k); it != nodes.end()) {
      order.splice(order.begin(), order, it->second);
      ++hits;
      continue;
    }
    if (order.size() == in.capacity) {
      nodes.erase(order.back());
      order.pop_back();
    }
    order.push_front(k);
    nodes.emplace(k, order.begin());
  }
  return {hits, in.keys.size() - hits, {order.rbegin(), order.rend()}};
}

}  // namespace

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

Outcome bst_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return bst(inserts, lookups); });
  return {r.checksum(), std::move(times), {}};
}

Outcome ordered_map_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return ordered_map(inserts, lookups); });
  return {r.checksum(), std::move(times), {}};
}

Outcome trie_kernel(const Args& args) {
  const auto [inserted, prefixes] = words(args.size);
  return outcome(measure(args.repeats, [&] { return trie(inserted, prefixes); }));
}

Outcome lru_kernel(const Args& args) {
  const auto input = lru_keys(args.size);
  auto [r, times] = measure(args.repeats, [&] { return lru(input); });
  return {std::format("{}:{}:{}", r.hits, r.misses, h31(r.oldest_first)), std::move(times), {}};
}

}  // namespace kernels
