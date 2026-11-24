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
namespace {

// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
std::uint32_t fmix32(std::uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6bu;
  h ^= h >> 13;
  h *= 0xc2b2ae35u;
  h ^= h >> 16;
  return h;
}

// bloom: k bit positions per key from two hashes

constexpr std::size_t kBloomBitsPerKey = 10;
constexpr std::uint32_t kBloomHashes = 7;
constexpr std::uint32_t kGolden = 0x9e3779b9u;

std::string bloom(std::span<const std::int64_t> keys, std::span<const std::int64_t> lookups) {
  const std::size_t m = kBloomBitsPerKey * keys.size();
  std::vector<std::uint8_t> bits((m + 7) / 8);
  const auto m32 = static_cast<std::uint32_t>(m);
  // The bit positions of a key: h1 + i*h2 (Kirsch-Mitzenmacher), wrapping at 2^32, modulo m.
  const auto position = [m32](std::int64_t key, std::uint32_t i) -> std::size_t {
    const auto k = static_cast<std::uint32_t>(key);
    const std::uint32_t h1 = fmix32(k);
    const std::uint32_t h2 = fmix32(k ^ kGolden) | 1;
    return (h1 + i * h2) % m32;
  };
  for (const std::int64_t key : keys) {
    for (std::uint32_t i = 0; i < kBloomHashes; ++i) {
      const std::size_t b = position(key, i);
      bits[b >> 3] |= static_cast<std::uint8_t>(1 << (b & 7));
    }
  }
  std::size_t false_positives = 0;
  for (const std::int64_t key : lookups) {
    bool all = true;
    for (std::uint32_t i = 0; i < kBloomHashes && all; ++i) {
      const std::size_t b = position(key, i);
      all = (bits[b >> 3] & (1 << (b & 7))) != 0;
    }
    false_positives += all ? 1 : 0;
  }
  std::uint64_t set_bits = 0;
  for (const std::uint8_t byte : bits) {
    set_bits += static_cast<std::uint64_t>(std::popcount(byte));
  }
  return std::format("{}:{}:{}", keys.size(), set_bits, false_positives);
}

// hyperloglog: the longest run of leading zeros estimates how many distinct items went by

constexpr int kHllP = 14;
constexpr std::size_t kHllM = std::size_t{1} << kHllP;

struct HllResult {
  std::uint64_t estimate;
  std::vector<std::uint8_t> registers;
};

HllResult hyperloglog(std::size_t n) {
  XorShift32 rng;
  const auto universe = static_cast<std::uint32_t>(std::max<std::size_t>(n / 4, 1));
  std::vector<std::uint8_t> registers(kHllM);
  for (std::size_t i = 0; i < n; ++i) {
    const std::uint32_t h = fmix32(rng.next() % universe);
    const std::uint32_t w = h << kHllP;
    const int rank = w == 0 ? 32 - kHllP + 1 : std::countl_zero(w) + 1;
    auto& reg = registers[h >> (32 - kHllP)];
    reg = std::max(reg, static_cast<std::uint8_t>(rank));
  }
  double z = 0.0;
  std::size_t zeros = 0;
  for (const std::uint8_t r : registers) {
    z += std::pow(2.0, -static_cast<int>(r));
    zeros += r == 0 ? 1 : 0;
  }
  const double m = static_cast<double>(kHllM);
  double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
  if (estimate <= 2.5 * m && zeros > 0) {
    estimate = m * std::log(m / static_cast<double>(zeros));
  }
  return {static_cast<std::uint64_t>(std::floor(estimate + 0.5)), std::move(registers)};
}

// skiplist: towers of random height turn a linked list into a search tree

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

// cycle: Floyd's tortoise and hare on x -> x*x + c mod M

constexpr std::uint64_t kCycleMod = 67108859;
constexpr std::uint64_t kCycleC = 12345;

std::uint64_t step(std::uint64_t x) { return (x * x + kCycleC) % kCycleMod; }

std::string cycle(const std::vector<std::uint64_t>& starts) {
  std::uint64_t total_mu = 0;
  std::uint64_t total_lam = 0;
  std::uint64_t longest = 0;
  for (const std::uint64_t x0 : starts) {
    std::uint64_t tortoise = step(x0);
    std::uint64_t hare = step(step(x0));
    while (tortoise != hare) {
      tortoise = step(tortoise);
      hare = step(step(hare));
    }
    std::uint64_t mu = 0;
    tortoise = x0;
    while (tortoise != hare) {
      tortoise = step(tortoise);
      hare = step(hare);
      ++mu;
    }
    std::uint64_t lam = 1;
    hare = step(tortoise);
    while (tortoise != hare) {
      hare = step(hare);
      ++lam;
    }
    total_mu += mu;
    total_lam += lam;
    longest = std::max(longest, mu + lam);
  }
  return std::format("{}:{}:{}:{}", starts.size(), total_mu, total_lam, longest);
}

// life: Conway's Game of Life on a torus

constexpr std::size_t kLifeSide = 512;

std::vector<std::uint8_t> life(const std::vector<std::uint8_t>& grid, std::size_t generations) {
  constexpr std::size_t w = kLifeSide;
  std::vector<std::uint8_t> cur = grid;
  std::vector<std::uint8_t> next(grid.size());
  for (std::size_t g = 0; g < generations; ++g) {
    for (std::size_t y = 0; y < w; ++y) {
      const std::size_t up = (y + w - 1) % w * w;
      const std::size_t row = y * w;
      const std::size_t down = (y + 1) % w * w;
      for (std::size_t x = 0; x < w; ++x) {
        const std::size_t left = (x + w - 1) % w;
        const std::size_t right = (x + 1) % w;
        const int n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] +
                      cur[row + right] + cur[down + left] + cur[down + x] + cur[down + right];
        next[row + x] = (n == 3 || (n == 2 && cur[row + x] == 1)) ? 1 : 0;
      }
    }
    std::swap(cur, next);
  }
  return cur;
}

// window-max: a monotonic deque keeps the maximum of every window in O(1) per step

constexpr std::array<std::size_t, 5> kWindows = {10, 100, 1000, 10000, 100000};

std::string window_max(const std::vector<std::int64_t>& values) {
  const std::size_t n = values.size();
  std::vector<std::size_t> dq(n);
  std::string out = std::to_string(n);
  for (const std::size_t w : kWindows) {
    std::size_t head = 0;
    std::size_t tail = 0;
    std::uint32_t h = 0;
    for (std::size_t i = 0; i < n; ++i) {
      const std::int64_t v = values[i];
      while (tail > head && values[dq[tail - 1]] <= v) {
        --tail;
      }
      dq[tail++] = i;
      if (dq[head] + w <= i) {
        ++head;
      }
      if (i + 1 >= w) {
        h = h * 31u + static_cast<std::uint32_t>(values[dq[head]]);
      }
    }
    out += std::format(":{}", h);
  }
  return out;
}

// rain-water: flood a 2D terrain from its walls inward with a min-heap

std::string rain_water(std::size_t side, const std::vector<std::uint64_t>& heights) {
  const std::size_t n = heights.size();
  std::vector<std::uint8_t> seen(n);
  std::priority_queue<std::uint64_t, std::vector<std::uint64_t>, std::greater<>> heap;
  for (std::size_t idx = 0; idx < n; ++idx) {
    const std::size_t r = idx / side;
    const std::size_t c = idx % side;
    if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
      seen[idx] = 1;
      heap.push(heights[idx] * n + idx);
    }
  }
  std::uint64_t water = 0;
  std::uint64_t wet = 0;
  while (!heap.empty()) {
    const std::uint64_t key = heap.top();
    heap.pop();
    const std::uint64_t level = key / n;
    const auto idx = static_cast<std::size_t>(key % n);
    const std::size_t r = idx / side;
    const std::size_t c = idx % side;
    const std::pair<std::size_t, bool> neighbours[] = {
        {idx - side, r > 0},
        {idx + side, r + 1 < side},
        {idx - 1, c > 0},
        {idx + 1, c + 1 < side},
    };
    for (const auto& [next, ok] : neighbours) {
      if (!ok || seen[next]) {
        continue;
      }
      seen[next] = 1;
      std::uint64_t h = heights[next];
      if (h < level) {
        water += level - h;
        ++wet;
        h = level;
      }
      heap.push(h * n + next);
    }
  }
  return std::format("{}:{}:{}", side, water, wet);
}

// collatz: remember every chain length below the start so each walk stops early

std::string collatz(std::size_t n) {
  std::vector<std::uint32_t> cache(n);
  cache[1] = 1;
  std::size_t best = 1;
  std::uint32_t best_len = 1;
  std::uint64_t total = 1;
  for (std::size_t start = 2; start < n; ++start) {
    std::uint64_t x = start;
    std::uint32_t steps = 0;
    while (x >= start) {
      x = x % 2 == 0 ? x / 2 : 3 * x + 1;
      ++steps;
    }
    const std::uint32_t len = steps + cache[x];
    cache[start] = len;
    total += len;
    if (len > best_len) {
      best = start;
      best_len = len;
    }
  }
  return std::format("{}:{}:{}:{}", n, best, best_len, total);
}

Outcome bloom_kernel(const Args& args) {
  const auto values = ints(2 * args.size);
  const std::span all(values);
  return outcome(
      measure(args.repeats, [&] { return bloom(all.first(args.size), all.subspan(args.size)); }));
}

Outcome hyperloglog_kernel(const Args& args) {
  auto [r, times] = measure(args.repeats, [&] { return hyperloglog(args.size); });
  return {std::format("{}:{}:{}", args.size, r.estimate, h31(r.registers)), std::move(times), {}};
}

Outcome skiplist_kernel(const Args& args) {
  const auto [keys, lookups] = hashmap_input(args.size);
  auto [r, times] = measure(args.repeats, [&] { return skiplist(keys, lookups); });
  return {std::format("{}:{}:{}:{}", r.keys, r.hits, r.list->heights(), r.list->keys_hash()),
          std::move(times),
          {}};
}

Outcome cycle_kernel(const Args& args) {
  XorShift32 rng;
  std::vector<std::uint64_t> starts(args.size);
  for (auto& s : starts) {
    s = rng.next() % kCycleMod;
  }
  return outcome(measure(args.repeats, [&] { return cycle(starts); }));
}

Outcome life_kernel(const Args& args) {
  XorShift32 rng;
  std::vector<std::uint8_t> grid(kLifeSide * kLifeSide);
  for (auto& cell : grid) {
    cell = rng.next() % 4 == 0 ? 1 : 0;
  }
  auto [cells, times] = measure(args.repeats, [&] { return life(grid, args.size); });
  std::size_t alive = 0;
  for (const std::uint8_t c : cells) {
    alive += c;
  }
  return {std::format("{}:{}:{}", args.size, alive, fnv1a(cells)), std::move(times), {}};
}

Outcome window_max_kernel(const Args& args) {
  const auto values = ints(args.size);
  return outcome(measure(args.repeats, [&] { return window_max(values); }));
}

Outcome rain_water_kernel(const Args& args) {
  const std::size_t side = args.size;
  XorShift32 rng;
  std::vector<std::uint64_t> heights(side * side);
  for (auto& h : heights) {
    h = rng.next() % 1000;
  }
  return outcome(measure(args.repeats, [&] { return rain_water(side, heights); }));
}

Outcome collatz_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return collatz(args.size); }));
}

constexpr std::array kPuzzleKernels = {
    Kernel{"sudoku", sudoku_kernel},
    Kernel{"bloom", bloom_kernel},
    Kernel{"hyperloglog", hyperloglog_kernel},
    Kernel{"skiplist", skiplist_kernel},
    Kernel{"cycle", cycle_kernel},
    Kernel{"palindrome", palindrome_kernel},
    Kernel{"bwt", bwt_kernel, "std::ranges::sort"},
    Kernel{"aho-corasick", aho_corasick_kernel},
    Kernel{"pi-digits", pi_digits_kernel, "Boost.Multiprecision 1.92 cpp_int"},
    Kernel{"life", life_kernel},
    Kernel{"huffman", huffman_kernel, "std::priority_queue"},
    Kernel{"window-max", window_max_kernel},
    Kernel{"rain-water", rain_water_kernel, "std::priority_queue"},
    Kernel{"collatz", collatz_kernel},
};

}  // namespace

std::span<const Kernel> puzzle_kernels() { return kPuzzleKernels; }

}  // namespace kernels
