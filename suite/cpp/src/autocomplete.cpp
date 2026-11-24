// Autocomplete kernels (../SPEC.md section 15): the first 10 words that start with a typed prefix,
// from a sorted list with binary search, a 26-slot pointer trie, or the same trie in one
// std::vector with 32-bit links. Reference: python/autocomplete.py.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {
namespace {

constexpr std::string_view kWordsFile = "words-web2.txt";
constexpr std::size_t kTopK = 10;
constexpr std::size_t kLetters = 26;

using Words = std::vector<std::string>;
using Results = std::vector<std::string_view>;

struct Shuffled {
  Words words;
  XorShift32 rng;
};

Shuffled shuffled_words(const Args& args) {
  const std::string text = read_file(args.data / kWordsFile);
  Words words;
  for (std::size_t start = 0; start < text.size();) {
    std::size_t end = text.find('\n', start);
    if (end == std::string::npos) {
      end = text.size();
    }
    std::string_view line(text.data() + start, end - start);
    if (line.ends_with('\r')) {
      line.remove_suffix(1);
    }
    if (!line.empty()) {
      words.emplace_back(line);
    }
    start = end + 1;
  }
  XorShift32 rng;
  for (std::size_t i = words.size() - 1; i > 0; --i) {
    std::swap(words[i], words[rng.next() % (i + 1)]);
  }
  return {std::move(words), rng};
}

Words build_input(const Args& args) {
  Words words = shuffled_words(args).words;
  words.resize(std::min(args.size, words.size()));
  return words;
}

std::pair<Words, Words> query_input(const Args& args) {
  auto [words, rng] = shuffled_words(args);
  Words queries;
  queries.reserve(args.size);
  for (std::size_t i = 0; i < args.size; ++i) {
    const std::string& w = words[rng.next() % words.size()];
    const std::size_t p = 2 + rng.next() % 4;
    queries.push_back(w.substr(0, std::min(p, w.size())));
  }
  return {std::move(words), std::move(queries)};
}

std::uint32_t fold(std::uint32_t h, std::string_view w) {
  h = h * 31u + static_cast<std::uint32_t>(w.size());
  return h * 31u + static_cast<std::uint8_t>(w.back());
}

template <class Range>
std::string build_check(const Range& in_order) {
  std::uint32_t h = 0;
  for (const auto& w : in_order) {
    h = fold(h, w);
  }
  return std::format("{}:{}", std::size(in_order), h);
}

void sorted_complete(const Words& sorted, std::string_view prefix, Results& out) {
  auto it = std::lower_bound(sorted.begin(), sorted.end(), prefix);
  const auto end = it + std::min<std::ptrdiff_t>(kTopK, sorted.end() - it);
  for (; it != end && it->starts_with(prefix); ++it) {
    out.push_back(*it);
  }
}

// mask (bit c set when children[c] is filled) is only filled in for the -mask kernels; it fits in
// the padding after word, so every pointer-trie kernel has the same 216-byte node.
struct TrieNode {
  std::array<std::unique_ptr<TrieNode>, kLetters> children;
  std::int32_t word = -1;
  std::uint32_t mask = 0;
};

struct Trie {
  std::unique_ptr<TrieNode> root;
  std::size_t nodes;
};

Trie trie_build(const Words& words) {
  Trie trie{std::make_unique<TrieNode>(), 1};
  for (std::size_t idx = 0; idx < words.size(); ++idx) {
    TrieNode* node = trie.root.get();
    for (const char ch : words[idx]) {
      auto& child = node->children[ch - 'a'];
      if (!child) {
        child = std::make_unique<TrieNode>();
        ++trie.nodes;
      }
      node = child.get();
    }
    node->word = static_cast<std::int32_t>(idx);
  }
  return trie;
}

void collect(const TrieNode& node, const Words& words, Results& out, std::size_t limit) {
  if (node.word >= 0) {
    out.push_back(words[node.word]);
  }
  for (const auto& child : node.children) {
    if (out.size() >= limit) {
      return;
    }
    if (child) {
      collect(*child, words, out, limit);
    }
  }
}

void trie_complete(const TrieNode& root, const Words& words, std::string_view prefix,
                   Results& out) {
  const TrieNode* node = &root;
  for (const char ch : prefix) {
    node = node->children[ch - 'a'].get();
    if (node == nullptr) {
      return;
    }
  }
  collect(*node, words, out, kTopK);
}

// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
struct ArenaNode {
  std::array<std::uint32_t, kLetters> children{};
  std::int32_t word = -1;
};

using Arena = std::vector<ArenaNode>;

Arena arena_build(const Words& words) {
  Arena nodes(1);
  for (std::size_t idx = 0; idx < words.size(); ++idx) {
    std::uint32_t node = 0;
    for (const char ch : words[idx]) {
      const std::size_t c = ch - 'a';
      if (nodes[node].children[c] == 0) {
        nodes[node].children[c] = static_cast<std::uint32_t>(nodes.size());
        nodes.emplace_back();
      }
      node = nodes[node].children[c];
    }
    nodes[node].word = static_cast<std::int32_t>(idx);
  }
  return nodes;
}

void arena_collect(const Arena& nodes, std::uint32_t node, const Words& words, Results& out,
                   std::size_t limit) {
  if (nodes[node].word >= 0) {
    out.push_back(words[nodes[node].word]);
  }
  for (const std::uint32_t child : nodes[node].children) {
    if (out.size() >= limit) {
      return;
    }
    if (child != 0) {
      arena_collect(nodes, child, words, out, limit);
    }
  }
}

void arena_complete(const Arena& nodes, const Words& words, std::string_view prefix, Results& out) {
  std::uint32_t node = 0;
  for (const char ch : prefix) {
    node = nodes[node].children[ch - 'a'];
    if (node == 0) {
      return;
    }
  }
  arena_collect(nodes, node, words, out, kTopK);
}

// The same arena renumbered in the order the depth-first walk visits it (a to z, parent before
// children), so a query's walk below its prefix reads nodes that sit next to each other. mask as in
// TrieNode (filled for both walk-order kernels, read only by ac-trie-dfs-mask).
struct WalkNode {
  std::array<std::uint32_t, kLetters> children{};
  std::int32_t word = -1;
  std::uint32_t mask = 0;
};

using WalkArena = std::vector<WalkNode>;

std::uint32_t walk_copy(const Arena& from, std::uint32_t node, WalkArena& to) {
  const auto at = static_cast<std::uint32_t>(to.size());
  to.emplace_back();
  to[at].word = from[node].word;
  for (std::size_t c = 0; c < kLetters; ++c) {
    if (from[node].children[c] != 0) {
      const std::uint32_t child = walk_copy(from, from[node].children[c], to);
      to[at].children[c] = child;
      to[at].mask |= 1u << c;
    }
  }
  return at;
}

WalkArena walk_order(const Arena& from) {
  WalkArena to;
  to.reserve(from.size());
  walk_copy(from, 0, to);
  return to;
}

void fill_masks(TrieNode& node) {
  for (std::size_t c = 0; c < kLetters; ++c) {
    if (node.children[c]) {
      node.mask |= 1u << c;
      fill_masks(*node.children[c]);
    }
  }
}

// One read-only view per structure, so the -mask walks, the prefix-only pass and the counting pass
// share one walk. Ref is a node handle; null() is "no child".
struct PointerView {
  using Ref = const TrieNode*;
  Ref root;
  Ref child(Ref n, std::size_t c) const { return n->children[c].get(); }
  static Ref null() { return nullptr; }
  std::int32_t word(Ref n) const { return n->word; }
  std::uint32_t mask(Ref n) const { return n->mask; }
};

template <class Node>
struct ArenaView {
  using Ref = std::uint32_t;
  const std::vector<Node>* nodes;
  Ref root = 0;
  Ref child(Ref n, std::size_t c) const { return (*nodes)[n].children[c]; }
  static Ref null() { return 0; }
  std::int32_t word(Ref n) const { return (*nodes)[n].word; }
  std::uint32_t mask(Ref n) const { return (*nodes)[n].mask; }
};

struct WalkStats {
  std::uint64_t nodes = 0;
  std::uint64_t slots = 0;
};

template <class View>
typename View::Ref prefix_node(const View& v, std::string_view prefix) {
  typename View::Ref node = v.root;
  for (const char ch : prefix) {
    node = v.child(node, ch - 'a');
    if (node == View::null()) {
      break;
    }
  }
  return node;
}

// The kernels' collect(): the node's own word, then its children a to z until ten results. Masked
// visits only filled slots (lowest set bit first, the same a-to-z order); stats counts nodes
// visited and slots read.
template <bool Masked, class View>
void view_collect(const View& v, typename View::Ref node, const Words& words, Results& out,
                  WalkStats* stats) {
  if (stats != nullptr) {
    ++stats->nodes;
  }
  if (v.word(node) >= 0) {
    out.push_back(words[v.word(node)]);
  }
  if constexpr (Masked) {
    for (std::uint32_t m = v.mask(node); m != 0; m &= m - 1) {
      if (out.size() >= kTopK) {
        return;
      }
      if (stats != nullptr) {
        ++stats->slots;
      }
      view_collect<Masked>(v, v.child(node, std::countr_zero(m)), words, out, stats);
    }
  } else {
    for (std::size_t c = 0; c < kLetters; ++c) {
      if (out.size() >= kTopK) {
        return;
      }
      if (stats != nullptr) {
        ++stats->slots;
      }
      const auto child = v.child(node, c);
      if (child != View::null()) {
        view_collect<Masked>(v, child, words, out, stats);
      }
    }
  }
}

template <bool Masked, class View>
void view_complete(const View& v, const Words& words, std::string_view prefix, Results& out,
                   WalkStats* stats = nullptr) {
  const auto node = prefix_node(v, prefix);
  if (node != View::null()) {
    view_collect<Masked>(v, node, words, out, stats);
  }
}

// Runs every query, folding each query's results into the hash inside the timed region.
template <class Complete>
std::string run_queries(const Words& queries, Complete complete) {
  std::size_t results = 0;
  std::uint32_t h = 0;
  Results out;
  out.reserve(kTopK);
  for (const auto& q : queries) {
    out.clear();
    complete(q, out);
    results += out.size();
    for (const std::string_view w : out) {
      h = fold(h, w);
    }
  }
  return std::format("{}:{}:{}", queries.size(), results, h);
}

double median_ms(std::size_t repeats, const auto& pass) {
  std::vector<double> ms(repeats);
  for (auto& m : ms) {
    const auto start = std::chrono::steady_clock::now();
    pass();
    m = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  }
  std::ranges::sort(ms);
  const std::size_t mid = ms.size() / 2;
  return ms.size() % 2 ? ms[mid] : (ms[mid - 1] + ms[mid]) / 2;
}

// Untimed by the harness, run after the timed repeats, each the median over the repeats:
// prefix_ms only finds where each prefix starts (lower bound, or the walk down to the prefix node);
// collect_ms only lists the first ten matches from those starts, found beforehand; twice_ms runs
// every query twice in a row (the second run finds its nodes in cache). Trie kernels add the
// nodes visited and slots read per query.
template <class Start, class Complete, class Find, class Collect>
std::string split_extra(const Args& args, const Words& queries, Complete complete, Find find,
                        Collect collect, const WalkStats* stats) {
  std::uintptr_t sink = 0;
  std::vector<Start> starts(queries.size());
  const double prefix_ms = median_ms(args.repeats, [&] {
    for (std::size_t i = 0; i < queries.size(); ++i) {
      starts[i] = find(queries[i]);
    }
  });
  Results out;
  out.reserve(kTopK);
  const double collect_ms = median_ms(args.repeats, [&] {
    for (std::size_t i = 0; i < queries.size(); ++i) {
      out.clear();
      collect(queries[i], starts[i], out);
      sink += out.size();
    }
  });
  const double twice_ms = median_ms(args.repeats, [&] {
    for (const auto& q : queries) {
      for (int k = 0; k < 2; ++k) {
        out.clear();
        complete(q, out);
        sink += out.size();
      }
    }
  });
  std::string extra =
      std::format(R"({{"prefix_ms":{:.3f},"collect_ms":{:.3f},"twice_ms":{:.3f},"sink":{})",
                  prefix_ms, collect_ms, twice_ms, sink % 2);
  if (stats != nullptr) {
    const auto n = static_cast<double>(queries.size());
    extra +=
        std::format(R"(,"nodes_per_query":{:.4f},"slots_per_query":{:.4f})",
                    static_cast<double>(stats->nodes) / n, static_cast<double>(stats->slots) / n);
  }
  return extra + "}";
}

template <bool Masked, class View>
WalkStats count_walk(const View& v, const Words& words, const Words& queries) {
  WalkStats stats;
  Results out;
  for (const auto& q : queries) {
    out.clear();
    view_complete<Masked>(v, words, q, out, &stats);
  }
  return stats;
}

Outcome with_extra(Outcome o, std::string extra) {
  o.extra = std::move(extra);
  return o;
}

Outcome sorted_build_kernel(const Args& args) {
  const Words words = build_input(args);
  auto [sorted, times] =
      measure_copy(args.repeats, words, [](Words& w) { std::sort(w.begin(), w.end()); });
  return {build_check(sorted), std::move(times), {}};
}

Outcome trie_build_check(const Results& walk, std::size_t nodes, std::vector<double> times) {
  return {build_check(walk), std::move(times), std::format("{{\"nodes\":{}}}", nodes)};
}

Outcome trie_build_kernel(const Args& args) {
  const Words words = build_input(args);
  auto [trie, times] = measure(args.repeats, [&] { return trie_build(words); });
  Results walk;
  collect(*trie.root, words, walk, words.size() + 1);
  return trie_build_check(walk, trie.nodes, std::move(times));
}

Outcome trie_arena_build_kernel(const Args& args) {
  const Words words = build_input(args);
  auto [nodes, times] = measure(args.repeats, [&] { return arena_build(words); });
  Results walk;
  arena_collect(nodes, 0, words, walk, words.size() + 1);
  return trie_build_check(walk, nodes.size(), std::move(times));
}

Outcome sorted_kernel(const Args& args) {
  auto [words, queries] = query_input(args);
  std::sort(words.begin(), words.end());
  const auto complete = [&](std::string_view q, Results& out) { sorted_complete(words, q, out); };
  Outcome o = outcome(measure(args.repeats, [&] { return run_queries(queries, complete); }));
  const auto find = [&](std::string_view q) {
    return std::lower_bound(words.begin(), words.end(), q);
  };
  const auto collect = [&](std::string_view q, Words::const_iterator it, Results& out) {
    const auto end = it + std::min<std::ptrdiff_t>(kTopK, words.cend() - it);
    for (; it != end && it->starts_with(q); ++it) {
      out.push_back(*it);
    }
  };
  return with_extra(std::move(o), split_extra<Words::const_iterator>(args, queries, complete, find,
                                                                     collect, nullptr));
}

// Times complete over the queries, then adds split_extra and the walk counts. collect(node, out)
// is the timed kernel's own walk below the prefix node.
template <bool Masked, class View, class Complete, class Collect>
Outcome trie_queries(const Args& args, const Words& words, const Words& queries, const View& v,
                     Complete complete, Collect collect) {
  Outcome o = outcome(measure(args.repeats, [&] { return run_queries(queries, complete); }));
  const WalkStats stats = count_walk<Masked>(v, words, queries);
  using Ref = typename View::Ref;
  const auto find = [&](std::string_view q) { return prefix_node(v, q); };
  const auto from = [&](std::string_view, Ref node, Results& out) {
    if (node != View::null()) {
      collect(node, out);
    }
  };
  return with_extra(std::move(o), split_extra<Ref>(args, queries, complete, find, from, &stats));
}

Outcome pointer_trie_kernel(const Args& args, bool az, bool masked) {
  auto [words, queries] = query_input(args);
  if (az) {
    std::ranges::sort(words);
  }
  Trie trie = trie_build(words);
  const PointerView v{trie.root.get()};
  if (masked) {
    fill_masks(*trie.root);
    return trie_queries<true>(
        args, words, queries, v,
        [&](std::string_view q, Results& out) { view_complete<true>(v, words, q, out); },
        [&](const TrieNode* n, Results& out) { view_collect<true>(v, n, words, out, nullptr); });
  }
  return trie_queries<false>(
      args, words, queries, v,
      [&](std::string_view q, Results& out) { trie_complete(*trie.root, words, q, out); },
      [&](const TrieNode* n, Results& out) { collect(*n, words, out, kTopK); });
}

Outcome trie_kernel(const Args& args) { return pointer_trie_kernel(args, false, false); }

// The same pointer trie built from the word list in A to Z order (the order the word file ships
// in), so nodes are allocated in the order a prefix walk visits them.
Outcome trie_sorted_kernel(const Args& args) { return pointer_trie_kernel(args, true, false); }

Outcome trie_mask_kernel(const Args& args) { return pointer_trie_kernel(args, false, true); }

Outcome trie_sorted_mask_kernel(const Args& args) { return pointer_trie_kernel(args, true, true); }

Outcome trie_arena_kernel(const Args& args) {
  const auto [words, queries] = query_input(args);
  const Arena nodes = arena_build(words);
  const ArenaView<ArenaNode> v{&nodes};
  return trie_queries<false>(
      args, words, queries, v,
      [&](std::string_view q, Results& out) { arena_complete(nodes, words, q, out); },
      [&](std::uint32_t n, Results& out) { arena_collect(nodes, n, words, out, kTopK); });
}

template <bool Masked>
Outcome trie_dfs_kernel(const Args& args) {
  const auto [words, queries] = query_input(args);
  const WalkArena nodes = walk_order(arena_build(words));
  const ArenaView<WalkNode> v{&nodes};
  return trie_queries<Masked>(
      args, words, queries, v,
      [&](std::string_view q, Results& out) { view_complete<Masked>(v, words, q, out); },
      [&](std::uint32_t n, Results& out) { view_collect<Masked>(v, n, words, out, nullptr); });
}

constexpr std::array kAutocompleteKernels{
    Kernel{"ac-sorted-build", sorted_build_kernel},
    Kernel{"ac-trie-build", trie_build_kernel},
    Kernel{"ac-trie-arena-build", trie_arena_build_kernel},
    Kernel{"ac-sorted", sorted_kernel},
    Kernel{"ac-trie", trie_kernel},
    Kernel{"ac-trie-sorted", trie_sorted_kernel},
    Kernel{"ac-trie-arena", trie_arena_kernel},
    Kernel{"ac-trie-mask", trie_mask_kernel},
    Kernel{"ac-trie-sorted-mask", trie_sorted_mask_kernel},
    Kernel{"ac-trie-dfs", trie_dfs_kernel<false>},
    Kernel{"ac-trie-dfs-mask", trie_dfs_kernel<true>},
};

}  // namespace

std::span<const Kernel> autocomplete_kernels() { return kAutocompleteKernels; }

}  // namespace kernels
