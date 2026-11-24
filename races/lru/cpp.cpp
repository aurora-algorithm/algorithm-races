// Race: lru (cpp)
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

Outcome lru_kernel(const Args& args) {
  const auto input = lru_keys(args.size);
  auto [r, times] = measure(args.repeats, [&] { return lru(input); });
  return {std::format("{}:{}:{}", r.hits, r.misses, h31(r.oldest_first)), std::move(times), {}};
}

}  // namespace kernels
