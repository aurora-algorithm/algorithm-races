// Race: ordered-map (cpp)
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

// distinct keys, hits, the keys in order (hashed after the timer)
struct TreeResult {
  std::size_t size;
  std::size_t hits;
  std::vector<std::int64_t> keys;

  std::string checksum() const { return std::format("{}:{}:{}", size, hits, h31(keys)); }
};

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

Outcome ordered_map_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return ordered_map(inserts, lookups); });
  return {r.checksum(), std::move(times), {}};
}

}  // namespace kernels
