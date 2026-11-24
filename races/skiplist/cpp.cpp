// Race: skiplist (cpp)
// Extracted from suite/cpp/src/puzzle.cpp — build and run via suite/run.py

// bloom, hyperloglog, skiplist, cycle, life, window-max, rain-water, collatz, and the puzzle
// dispatch table
// (../SPEC.md section 13).
#include "puzzle.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace kernels {

constexpr std::size_t kSkipMaxLevel = 24;

constexpr std::uint32_t kSkipLevelSeed = 88675123u;

struct SkipNode {
  std::int64_t key;
  std::vector<SkipNode*> next;
};

class SkipList {
 public:
  SkipList() {
    nodes_.push_back(std::make_unique<SkipNode>(-1, std::vector<SkipNode*>(kSkipMaxLevel)));
  }

  // Walks from the top level down to the last node with a smaller key, noting where it dropped a
  // level.
  SkipNode* find(std::int64_t key, std::array<SkipNode*, kSkipMaxLevel>& update) const {
    SkipNode* x = nodes_[0].get();
    for (std::size_t i = level_; i-- > 0;) {
      while (x->next[i] != nullptr && x->next[i]->key < key) {
        x = x->next[i];
      }
      update[i] = x;
    }
    SkipNode* next = x->next[0];
    return next != nullptr && next->key == key ? next : nullptr;
  }

  bool contains(std::int64_t key) const {
    std::array<SkipNode*, kSkipMaxLevel> update{};
    return find(key, update) != nullptr;
  }

  void insert(std::int64_t key, XorShift32& rng) {
    std::array<SkipNode*, kSkipMaxLevel> update;
    update.fill(nodes_[0].get());
    if (find(key, update) != nullptr) {
      return;
    }
    std::size_t height = 1;
    while (height < kSkipMaxLevel && (rng.next() & 1) == 1) {
      ++height;
    }
    level_ = std::max(level_, height);
    auto node = std::make_unique<SkipNode>(key, std::vector<SkipNode*>(height));
    for (std::size_t i = 0; i < height; ++i) {
      node->next[i] = update[i]->next[i];
      update[i]->next[i] = node.get();
    }
    nodes_.push_back(std::move(node));
    heights_ += height;
  }

  std::uint32_t keys_hash() const {
    std::uint32_t h = 0;
    for (const SkipNode* x = nodes_[0]->next[0]; x != nullptr; x = x->next[0]) {
      h = h * 31u + static_cast<std::uint32_t>(x->key);
    }
    return h;
  }

  std::size_t heights() const { return heights_; }

 private:
  std::vector<std::unique_ptr<SkipNode>> nodes_;
  std::size_t level_ = 1;
  std::size_t heights_ = 0;
};

struct SkipResult {
  std::unique_ptr<SkipList> list;
  std::size_t keys;
  std::size_t hits;
};

SkipResult skiplist(const std::vector<std::int64_t>& keys,
                    const std::vector<std::int64_t>& lookups) {
  auto list = std::make_unique<SkipList>();
  XorShift32 rng(kSkipLevelSeed);
  for (const std::int64_t key : keys) {
    list->insert(key, rng);
  }
  std::size_t hits = 0;
  for (const std::int64_t key : lookups) {
    hits += list->contains(key) ? 1 : 0;
  }
  return {std::move(list), keys.size(), hits};
}

Outcome skiplist_kernel(const Args& args) {
  const auto [keys, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return skiplist(keys, lookups); });
  return {std::format("{}:{}:{}:{}", r.keys, r.hits, r.list->heights(), r.list->keys_hash()),
          std::move(times),
          {}};
}

}  // namespace kernels
