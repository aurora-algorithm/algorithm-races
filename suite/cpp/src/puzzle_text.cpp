// The string kernels of group `puzzle` (../SPEC.md section 13): palindrome, bwt, aho-corasick,
// huffman.
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <numeric>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "puzzle.hpp"

namespace kernels {
namespace {

constexpr std::size_t kOneMib = std::size_t{1} << 20;
constexpr std::string_view kDna = "ACGT";

std::vector<std::uint8_t> repeat(const std::vector<std::uint8_t>& block, std::size_t copies) {
  std::vector<std::uint8_t> out;
  out.reserve(block.size() * copies);
  for (std::size_t i = 0; i < copies; ++i) {
    out.insert(out.end(), block.begin(), block.end());
  }
  return out;
}

// palindrome: Manacher's mirror trick finds every palindrome in linear time

// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
std::vector<std::uint8_t> binary_text(std::size_t n) {
  XorShift32 rng;
  std::vector<std::uint8_t> out(n);
  for (std::size_t start = 0; start < n; start += 32) {
    std::uint32_t v = rng.next();
    for (std::size_t i = start; i < std::min(n, start + 32); ++i) {
      out[i] = (v & 1) == 1 ? 'b' : 'a';
      v >>= 1;
    }
  }
  return out;
}

std::string palindrome(const std::vector<std::uint8_t>& text) {
  std::vector<std::uint8_t> t(2 * text.size() + 3, '#');
  const std::size_t last = t.size() - 1;
  t[0] = '^';
  t[last] = '$';
  for (std::size_t i = 0; i < text.size(); ++i) {
    t[2 + 2 * i] = text[i];
  }
  std::vector<std::size_t> p(t.size());
  std::size_t center = 0;
  std::size_t right = 0;
  for (std::size_t i = 1; i < last; ++i) {
    if (i < right) {
      p[i] = std::min(right - i, p[2 * center - i]);
    }
    while (t[i + 1 + p[i]] == t[i - 1 - p[i]]) {
      ++p[i];
    }
    if (i + p[i] > right) {
      center = i;
      right = i + p[i];
    }
  }
  std::size_t longest = 0;
  std::size_t best = 0;
  std::size_t count = 0;
  for (std::size_t i = 0; i < p.size(); ++i) {
    count += (p[i] + 1) / 2;
    if (p[i] > longest) {
      longest = p[i];
      best = i;
    }
  }
  return std::format("{}:{}:{}:{}", text.size(), longest, (best - 1 - longest) / 2, count);
}

// bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
std::vector<std::uint8_t> dna_from(XorShift32& rng, std::size_t n) {
  std::vector<std::uint8_t> out(n);
  for (std::size_t start = 0; start < n; start += 16) {
    std::uint32_t v = rng.next();
    for (std::size_t i = start; i < std::min(n, start + 16); ++i) {
      out[i] = static_cast<std::uint8_t>(kDna[v & 3]);
      v >>= 2;
    }
  }
  return out;
}

struct BwtResult {
  std::size_t n;
  int rounds;
  std::size_t primary;
  std::vector<std::uint8_t> out;
};

BwtResult bwt(const std::vector<std::uint8_t>& dna) {
  std::vector<std::uint8_t> text = dna;
  text.push_back('$');
  const std::size_t n = text.size();
  std::vector<std::int32_t> rank(text.begin(), text.end());
  std::vector<std::size_t> sa(n);
  std::iota(sa.begin(), sa.end(), std::size_t{0});
  std::vector<std::int32_t> second(n);
  std::vector<std::int32_t> tmp(n);
  std::size_t k = 1;
  int rounds = 0;
  while (true) {
    ++rounds;
    for (std::size_t i = 0; i < n; ++i) {
      second[i] = i + k < n ? rank[i + k] : -1;
    }
    std::ranges::sort(sa, [&](std::size_t a, std::size_t b) {
      return std::tie(rank[a], second[a]) < std::tie(rank[b], second[b]);
    });
    tmp[sa[0]] = 0;
    for (std::size_t j = 1; j < n; ++j) {
      const std::size_t a = sa[j - 1];
      const std::size_t b = sa[j];
      const bool differs = rank[a] != rank[b] || second[a] != second[b];
      tmp[b] = tmp[a] + (differs ? 1 : 0);
    }
    std::swap(rank, tmp);
    if (static_cast<std::size_t>(rank[sa[n - 1]]) == n - 1) {
      break;
    }
    k *= 2;
  }
  std::size_t primary = 0;
  std::vector<std::uint8_t> out(n);
  for (std::size_t j = 0; j < n; ++j) {
    if (sa[j] == 0) {
      primary = j;
      out[j] = text[n - 1];
    } else {
      out[j] = text[sa[j] - 1];
    }
  }
  return {n, rounds, primary, std::move(out)};
}

// aho-corasick: a trie with failure links matches thousands of patterns in one pass

constexpr std::size_t kAhoPatterns = 10000;

struct AhoInput {
  std::vector<std::vector<std::uint8_t>> patterns;
  std::vector<std::uint8_t> text;
};

AhoInput aho_input(std::size_t n) {
  XorShift32 rng;
  std::vector<std::vector<std::uint8_t>> patterns(kAhoPatterns);
  for (auto& pattern : patterns) {
    pattern.resize(6 + rng.next() % 7);
    for (auto& ch : pattern) {
      ch = static_cast<std::uint8_t>(kDna[rng.next() % 4]);
    }
  }
  const auto block = dna_from(rng, kOneMib);
  return {std::move(patterns), repeat(block, n / kOneMib)};
}

std::string aho_corasick(const AhoInput& in) {
  std::array<std::size_t, 256> code{};
  for (std::size_t i = 0; i < kDna.size(); ++i) {
    code[static_cast<std::uint8_t>(kDna[i])] = i;
  }
  std::vector<std::int32_t> go(4, -1);
  std::vector<std::uint64_t> out(1);
  for (const auto& pattern : in.patterns) {
    std::size_t s = 0;
    for (const std::uint8_t ch : pattern) {
      const std::size_t edge = s * 4 + code[ch];
      if (go[edge] == -1) {
        go[edge] = static_cast<std::int32_t>(out.size());
        go.insert(go.end(), 4, -1);
        out.push_back(0);
      }
      s = static_cast<std::size_t>(go[edge]);
    }
    ++out[s];
  }
  std::vector<std::size_t> fail(out.size());
  std::vector<std::size_t> queue;
  queue.reserve(out.size());
  for (std::size_t c = 0; c < 4; ++c) {
    if (go[c] == -1) {
      go[c] = 0;
    } else {
      queue.push_back(static_cast<std::size_t>(go[c]));
    }
  }
  for (std::size_t head = 0; head < queue.size(); ++head) {
    const std::size_t s = queue[head];
    out[s] += out[fail[s]];
    for (std::size_t c = 0; c < 4; ++c) {
      const std::int32_t fallback = go[fail[s] * 4 + c];
      const std::int32_t t = go[s * 4 + c];
      if (t == -1) {
        go[s * 4 + c] = fallback;
      } else {
        fail[static_cast<std::size_t>(t)] = static_cast<std::size_t>(fallback);
        queue.push_back(static_cast<std::size_t>(t));
      }
    }
  }
  std::size_t s = 0;
  std::uint64_t matches = 0;
  std::uint64_t hits = 0;
  for (const std::uint8_t ch : in.text) {
    s = static_cast<std::size_t>(go[s * 4 + code[ch]]);
    if (out[s] > 0) {
      matches += out[s];
      ++hits;
    }
  }
  return std::format("{}:{}:{}:{}", in.patterns.size(), out.size(), matches, hits);
}

// huffman: merge the two rarest symbols until one tree is left

// A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
std::vector<std::uint8_t> skewed_text(std::size_t n) {
  XorShift32 rng;
  std::vector<std::uint8_t> block(kOneMib);
  for (auto& ch : block) {
    const std::uint32_t a = rng.next() % 26;
    const std::uint32_t b = rng.next() % 26;
    ch = static_cast<std::uint8_t>(97 + a * b / 26);
  }
  return repeat(block, n / kOneMib);
}

// Children of the internal nodes 256, 257, ...; ids below 256 are leaves (the byte itself).
struct Tree {
  std::array<std::size_t, 512> left{};
  std::array<std::size_t, 512> right{};
  std::size_t root = 0;
};

Tree huffman_tree(const std::array<std::uint64_t, 256>& freq) {
  std::priority_queue<std::uint64_t, std::vector<std::uint64_t>, std::greater<>> heap;
  for (std::uint64_t sym = 0; sym < 256; ++sym) {
    if (freq[sym] > 0) {
      heap.push(freq[sym] * 1024 + sym);
    }
  }
  Tree tree;
  std::size_t next_id = 256;
  while (heap.size() > 1) {
    const std::uint64_t a = heap.top();
    heap.pop();
    const std::uint64_t b = heap.top();
    heap.pop();
    tree.left[next_id] = static_cast<std::size_t>(a % 1024);
    tree.right[next_id] = static_cast<std::size_t>(b % 1024);
    heap.push((a / 1024 + b / 1024) * 1024 + next_id);
    ++next_id;
  }
  tree.root = heap.empty() ? 0 : static_cast<std::size_t>(heap.top() % 1024);
  return tree;
}

struct Codes {
  std::array<std::uint64_t, 256> code{};
  std::array<std::uint32_t, 256> length{};
};

Codes huffman_codes(const Tree& tree) {
  Codes codes;
  std::vector<std::tuple<std::size_t, std::uint64_t, std::uint32_t>> stack = {{tree.root, 0, 0}};
  while (!stack.empty()) {
    const auto [node, code, len] = stack.back();
    stack.pop_back();
    if (node < 256) {
      codes.code[node] = code;
      codes.length[node] = len;
    } else {
      stack.emplace_back(tree.left[node], code << 1, len + 1);
      stack.emplace_back(tree.right[node], (code << 1) | 1, len + 1);
    }
  }
  return codes;
}

std::pair<std::uint64_t, std::vector<std::uint8_t>> huffman_encode(
    const std::vector<std::uint8_t>& text, const Codes& codes) {
  std::vector<std::uint8_t> encoded;
  std::uint64_t acc = 0;
  std::uint32_t nbits = 0;
  std::uint64_t bits = 0;
  for (const std::uint8_t b : text) {
    const std::uint32_t len = codes.length[b];
    acc = (acc << len) | codes.code[b];
    nbits += len;
    bits += len;
    while (nbits >= 8) {
      nbits -= 8;
      encoded.push_back(static_cast<std::uint8_t>(acc >> nbits));
    }
    acc &= (std::uint64_t{1} << nbits) - 1;
  }
  if (nbits > 0) {
    encoded.push_back(static_cast<std::uint8_t>(acc << (8 - nbits)));
  }
  return {bits, std::move(encoded)};
}

std::vector<std::uint8_t> huffman_decode(const Tree& tree, const std::vector<std::uint8_t>& encoded,
                                         std::size_t n) {
  std::vector<std::uint8_t> decoded;
  decoded.reserve(n);
  std::size_t node = tree.root;
  for (const std::uint8_t byte : encoded) {
    for (int shift = 7; shift >= 0; --shift) {
      node = ((byte >> shift) & 1) == 1 ? tree.right[node] : tree.left[node];
      if (node < 256) {
        decoded.push_back(static_cast<std::uint8_t>(node));
        node = tree.root;
        if (decoded.size() == n) {
          return decoded;
        }
      }
    }
  }
  return decoded;
}

struct HuffmanResult {
  std::uint64_t bits;
  std::vector<std::uint8_t> encoded;
  std::vector<std::uint8_t> decoded;
};

HuffmanResult huffman(const std::vector<std::uint8_t>& text) {
  std::array<std::uint64_t, 256> freq{};
  for (const std::uint8_t b : text) {
    ++freq[b];
  }
  const Tree tree = huffman_tree(freq);
  const Codes codes = huffman_codes(tree);
  auto [bits, encoded] = huffman_encode(text, codes);
  auto decoded = huffman_decode(tree, encoded, text.size());
  return {bits, std::move(encoded), std::move(decoded)};
}

}  // namespace

Outcome palindrome_kernel(const Args& args) {
  const auto text = binary_text(args.size);
  return outcome(measure(args.repeats, [&] { return palindrome(text); }));
}

Outcome bwt_kernel(const Args& args) {
  XorShift32 rng;
  const auto dna = dna_from(rng, args.size);
  auto [r, times] = measure(args.repeats, [&] { return bwt(dna); });
  return {std::format("{}:{}:{}:{}", r.n, r.rounds, r.primary, fnv1a(r.out)), std::move(times), {}};
}

Outcome aho_corasick_kernel(const Args& args) {
  const auto input = aho_input(args.size);
  return outcome(measure(args.repeats, [&] { return aho_corasick(input); }));
}

Outcome huffman_kernel(const Args& args) {
  const auto text = skewed_text(args.size);
  auto [r, times] = measure(args.repeats, [&] { return huffman(text); });
  return {
      std::format("{}:{}:{}:{}", text.size(), r.bits, fnv1a(r.encoded), r.decoded == text ? 1 : 0),
      std::move(times),
      {}};
}

}  // namespace kernels
