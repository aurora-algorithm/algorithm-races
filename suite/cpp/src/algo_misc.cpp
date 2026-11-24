// edit-distance, knapsack, lis, kmp, str-find, fib, nqueens, binary-search, and the algo dispatch
// table
// (../SPEC.md section 12).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "algo.hpp"
#include "inputs.hpp"

namespace kernels {
namespace {

constexpr std::string_view kAcgt = "acgt";
constexpr std::size_t kKnapsackItems = 1000;
constexpr std::size_t kPatternLen = 8;

std::vector<std::string> dna(std::size_t length, std::size_t count) {
  XorShift32 rng;
  std::vector<std::string> out(count);
  for (auto& s : out) {
    s.resize(length);
    for (auto& ch : s) {
      ch = kAcgt[rng.next() % 4];
    }
  }
  return out;
}

std::string edit_distance(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min({prev[j + 1] + 1, cur[j] + 1, prev[j] + cost});
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

// edit_distance with two nested two-argument std::min calls in place of the initializer list
// (runtime variants cpp-*-edmin2, KERNELS_ED_MIN=min2).
std::string edit_distance_min2(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min(std::min(prev[j + 1] + 1, cur[j] + 1), prev[j] + cost);
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

// edit_distance_min2 with the left neighbour (cur[j]) compared last, the order of the C variant
// c-clang-edleft (runtime variants cpp-*-edmin2l, KERNELS_ED_MIN=min2l).
std::string edit_distance_min2l(const std::string& a, const std::string& b) {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> cur(b.size() + 1);
  for (std::size_t j = 0; j < prev.size(); ++j) {
    prev[j] = j;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    cur[0] = i + 1;
    for (std::size_t j = 0; j < b.size(); ++j) {
      const std::size_t cost = a[i] != b[j] ? 1 : 0;
      cur[j + 1] = std::min(std::min(prev[j + 1] + 1, prev[j] + cost), cur[j] + 1);
    }
    std::swap(prev, cur);
  }
  return std::to_string(prev[b.size()]);
}

bool env_is(const char* name, std::string_view value) {
  const char* got =
      std::getenv(name);  // NOLINT(concurrency-mt-unsafe): read once, before any thread
  return got != nullptr && value == got;
}

struct Item {
  std::size_t weight;
  std::uint64_t value;
};

std::vector<Item> items() {
  XorShift32 rng;
  std::vector<Item> out(kKnapsackItems);
  for (Item& it : out) {
    it.weight = 1 + rng.next() % 1000;
    it.value = 1 + rng.next() % 1000;
  }
  return out;
}

std::vector<std::uint64_t> knapsack(std::size_t capacity, const std::vector<Item>& things) {
  std::vector<std::uint64_t> best(capacity + 1);
  for (const auto& [weight, value] : things) {
    for (std::size_t c = capacity; c >= weight; --c) {
      if (best[c - weight] + value > best[c]) {
        best[c] = best[c - weight] + value;
      }
    }
  }
  return best;
}

std::vector<std::int64_t> lis(const std::vector<std::int64_t>& values) {
  std::vector<std::int64_t> tails;
  for (const std::int64_t x : values) {
    const auto it = std::ranges::lower_bound(tails, x);
    if (it == tails.end()) {
      tails.push_back(x);
    } else {
      *it = x;
    }
  }
  return tails;
}

struct Matches {
  std::size_t count = 0;
  std::int64_t first = -1;
  std::int64_t last = -1;

  void add(std::size_t pos) {
    const auto p = static_cast<std::int64_t>(pos);
    if (first < 0) {
      first = p;
    }
    last = p;
    ++count;
  }

  std::string str() const { return std::format("{}:{}:{}", count, first, last); }
};

// The 64 most common English words (../SPEC.md section 12, kmp-en).
constexpr std::array<std::string_view, 64> kWords = {
    "the",   "be",   "to",   "of",    "and",   "a",      "in",   "that", "have", "i",    "it",
    "for",   "not",  "on",   "with",  "he",    "as",     "you",  "do",   "at",   "this", "but",
    "his",   "by",   "from", "they",  "we",    "say",    "her",  "she",  "or",   "an",   "will",
    "my",    "one",  "all",  "would", "there", "their",  "what", "so",   "up",   "out",  "if",
    "about", "who",  "get",  "which", "go",    "me",     "when", "make", "can",  "like", "time",
    "no",    "just", "him",  "know",  "take",  "people", "into", "year", "your"};

std::string english(std::size_t n) {
  XorShift32 rng;
  std::string t;
  t.reserve(n + 16);
  while (t.size() < n) {
    const std::uint32_t a = rng.next() % 64;
    const std::uint32_t b = rng.next() % 64;
    t += kWords[std::min(a, b)];
    const std::uint32_t r = rng.next() % 16;
    t += r == 0 ? ". " : r == 1 ? ", " : " ";
  }
  t.resize(n);
  return t;
}

std::pair<std::string, std::string> text_en(std::size_t n) {
  auto t = english(n);
  auto p = t.substr(n / 2, kPatternLen);
  return {std::move(t), std::move(p)};
}

std::pair<std::string, std::string> text(std::size_t n) {
  auto t = std::move(dna(n, 1)[0]);
  auto p = t.substr(n / 2, kPatternLen);
  return {std::move(t), std::move(p)};
}

Matches kmp(std::string_view t, std::string_view p) {
  const std::size_t m = p.size();
  std::vector<std::size_t> fail(m);
  std::size_t k = 0;
  for (std::size_t i = 1; i < m; ++i) {
    while (k > 0 && p[i] != p[k]) {
      k = fail[k - 1];
    }
    if (p[i] == p[k]) {
      ++k;
    }
    fail[i] = k;
  }
  Matches found;
  k = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    while (k > 0 && t[i] != p[k]) {
      k = fail[k - 1];
    }
    if (t[i] == p[k]) {
      ++k;
    }
    if (k == m) {
      found.add(i + 1 - m);
      k = fail[k - 1];
    }
  }
  return found;
}

Matches str_find(std::string_view t, std::string_view p) {
  Matches found;
  for (std::size_t pos = t.find(p); pos != std::string_view::npos; pos = t.find(p, pos + 1)) {
    found.add(pos);
  }
  return found;
}

// The same loop with std::boyer_moore_horspool_searcher (runtime variants cpp-*-bmh); the
// searcher's tables are built inside the timed call, as a caller building one per search would.
Matches str_find_bmh(std::string_view t, std::string_view p) {
  Matches found;
  const std::boyer_moore_horspool_searcher searcher(p.begin(), p.end());
  auto at = t.begin();
  while (true) {
    const auto hit = std::search(at, t.end(), searcher);
    if (hit == t.end()) {
      return found;
    }
    const auto pos = static_cast<std::size_t>(hit - t.begin());
    found.add(pos);
    at = t.begin() + static_cast<std::ptrdiff_t>(pos + 1);
  }
}

// The same loop with C's strstr on the same NUL-terminated string (runtime variants cpp-*-strstr).
Matches str_find_strstr(const std::string& t, const std::string& p) {
  Matches found;
  const char* base = t.c_str();
  for (const char* at = std::strstr(base, p.c_str()); at != nullptr;
       at = std::strstr(at + 1, p.c_str())) {
    found.add(static_cast<std::size_t>(at - base));
  }
  return found;
}

// Positions whose letter is the pattern's first letter but where no match starts: each one is a
// first-letter hit that the find loop then rejects with a compare. Counted after timing.
std::string false_starts(std::string_view t, std::string_view p, std::size_t matches) {
  const std::size_t hits = static_cast<std::size_t>(std::count(t.begin(), t.end(), p[0]));
  return std::format(R"({{"first_letter_hits":{},"false_starts":{}}})", hits, hits - matches);
}

std::uint64_t fib(std::uint32_t n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

struct Board {
  std::size_t n;
  std::vector<std::uint8_t> cols;
  std::vector<std::uint8_t> up;
  std::vector<std::uint8_t> down;

  std::uint64_t place(std::size_t row) {
    if (row == n) {
      return 1;
    }
    std::uint64_t count = 0;
    for (std::size_t col = 0; col < n; ++col) {
      const std::size_t u = row + col;
      const std::size_t d = row + n - 1 - col;
      if (!cols[col] && !up[u] && !down[d]) {
        cols[col] = up[u] = down[d] = 1;
        count += place(row + 1);
        cols[col] = up[u] = down[d] = 0;
      }
    }
    return count;
  }
};

std::uint64_t nqueens(std::size_t n) {
  Board board{n, std::vector<std::uint8_t>(n), std::vector<std::uint8_t>(2 * n - 1),
              std::vector<std::uint8_t>(2 * n - 1)};
  return board.place(0);
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> sorted_array(std::size_t n) {
  XorShift32 rng;
  std::vector<std::int64_t> a(n);
  std::int64_t v = 0;
  for (auto& x : a) {
    v += 1 + rng.next() % 4;
    x = v;
  }
  std::vector<std::int64_t> queries(n);
  for (auto& q : queries) {
    q = static_cast<std::int64_t>(rng.next()) % (v + 1);
  }
  return {std::move(a), std::move(queries)};
}

std::string binary_search(const std::vector<std::int64_t>& a,
                          const std::vector<std::int64_t>& queries) {
  const std::size_t n = a.size();
  std::uint64_t hits = 0;
  std::uint64_t total = 0;
  for (const std::int64_t q : queries) {
    std::size_t lo = 0;
    std::size_t hi = n;
    while (lo < hi) {
      const std::size_t mid = (lo + hi) / 2;
      if (a[mid] < q) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    if (lo < n && a[lo] == q) {
      ++hits;
    }
    total += lo;
  }
  return std::format("{}:{}", hits, total);
}

Outcome edit_distance_kernel(const Args& args) {
  const auto ab = dna(args.size, 2);
  if (env_is("KERNELS_ED_MIN", "min2")) {
    return outcome(measure(args.repeats, [&] { return edit_distance_min2(ab[0], ab[1]); }));
  }
  if (env_is("KERNELS_ED_MIN", "min2l")) {
    return outcome(measure(args.repeats, [&] { return edit_distance_min2l(ab[0], ab[1]); }));
  }
  return outcome(measure(args.repeats, [&] { return edit_distance(ab[0], ab[1]); }));
}

Outcome knapsack_kernel(const Args& args) {
  const auto things = items();
  auto [best, times] = measure(args.repeats, [&] { return knapsack(args.size, things); });
  return {std::format("{}:{}", best.back(), h31(best)), std::move(times), {}};
}

Outcome lis_kernel(const Args& args) {
  const auto values = ints(args.size);
  auto [tails, times] = measure(args.repeats, [&] { return lis(values); });
  return {std::format("{}:{}", tails.size(), h31(tails)), std::move(times), {}};
}

Outcome kmp_kernel(const Args& args) {
  const auto [t, p] = text(args.size);
  auto [found, times] = measure(args.repeats, [&] { return kmp(t, p); });
  return {found.str(), std::move(times), {}};
}

// KERNELS_STRFIND=bmh or strstr swaps the search; the default std::string_view::find row also
// reports its false starts in extra.
Outcome search_kernel(const std::string& t, const std::string& p, std::size_t repeats) {
  if (env_is("KERNELS_STRFIND", "bmh")) {
    auto [found, times] = measure(repeats, [&] { return str_find_bmh(t, p); });
    return {found.str(), std::move(times), {}};
  }
  if (env_is("KERNELS_STRFIND", "strstr")) {
    auto [found, times] = measure(repeats, [&] { return str_find_strstr(t, p); });
    return {found.str(), std::move(times), {}};
  }
  auto [found, times] = measure(repeats, [&] { return str_find(t, p); });
  return {found.str(), std::move(times), false_starts(t, p, found.count)};
}

Outcome str_find_kernel(const Args& args) {
  const auto [t, p] = text(args.size);
  return search_kernel(t, p, args.repeats);
}

Outcome kmp_en_kernel(const Args& args) {
  const auto [t, p] = text_en(args.size);
  auto [found, times] = measure(args.repeats, [&] { return kmp(t, p); });
  return {found.str(), std::move(times), {}};
}

Outcome str_find_en_kernel(const Args& args) {
  const auto [t, p] = text_en(args.size);
  return search_kernel(t, p, args.repeats);
}

Outcome fib_kernel(const Args& args) {
  const auto n = static_cast<std::uint32_t>(args.size);
  return outcome(measure(args.repeats, [n] { return std::to_string(fib(n)); }));
}

Outcome nqueens_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return std::to_string(nqueens(args.size)); }));
}

Outcome binary_search_kernel(const Args& args) {
  const auto [a, queries] = sorted_array(args.size);
  return outcome(measure(args.repeats, [&] { return binary_search(a, queries); }));
}

constexpr std::array kAlgoKernels = {
    Kernel{"bfs-grid", bfs_grid_kernel, "std::queue"},
    Kernel{"dijkstra", dijkstra_kernel, "std::priority_queue"},
    Kernel{"kruskal", kruskal_kernel, "std::ranges::sort"},
    Kernel{"bst", bst_kernel},
    Kernel{"ordered-map", ordered_map_kernel, "std::map"},
    Kernel{"trie", trie_kernel},
    Kernel{"lru", lru_kernel, "std::list + std::unordered_map"},
    Kernel{"edit-distance", edit_distance_kernel},
    Kernel{"knapsack", knapsack_kernel},
    Kernel{"lis", lis_kernel, "std::ranges::lower_bound"},
    Kernel{"kmp", kmp_kernel},
    Kernel{"str-find", str_find_kernel, "std::string_view::find"},
    Kernel{"kmp-en", kmp_en_kernel},
    Kernel{"str-find-en", str_find_en_kernel, "std::string_view::find"},
    Kernel{"fib", fib_kernel},
    Kernel{"nqueens", nqueens_kernel},
    Kernel{"binary-search", binary_search_kernel},
};

}  // namespace

std::span<const Kernel> algo_kernels() { return kAlgoKernels; }

}  // namespace kernels
