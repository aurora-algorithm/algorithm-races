// The builtins group's many-words jobs (../../builtins-NOTES.md, "Many words"): find every place
// any of K words occurs in a log. C++ has std::string_view::find and std::regex, and no
// Aho-Corasick in its standard library, so the third way is written by hand (the same table as
// the C kernel). Every way returns "{K}:{matches}:{sum of match start offsets}"; the words are
// 8-digit ids hidden inside longer digit runs (session tokens). They are never a suffix of one
// another, and no two matches overlap in these logs.
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <queue>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {
namespace {

constexpr std::uint32_t kPool = 10000;
constexpr std::array<std::string_view, 4> kMethods{"GET", "POST", "PUT", "DELETE"};
constexpr std::array<unsigned, 12> kStatuses{200, 201, 204, 301, 304, 400,
                                             401, 403, 404, 500, 502, 503};

struct Found {
  std::uint64_t matches = 0;
  std::uint64_t offsets = 0;
};

struct Input {
  std::string text;
  std::vector<std::string> words;
};

// 0 to 6 random digits: the length first, then each digit.
std::string random_digits(XorShift32& rng) {
  const std::uint32_t n = rng.next() % 7;
  std::string out;
  for (std::uint32_t i = 0; i < n; ++i) {
    out += static_cast<char>('0' + rng.next() % 10);
  }
  return out;
}

Input words_input(std::size_t lines, std::size_t k) {
  XorShift32 rng;
  std::vector<std::string> pool;
  pool.reserve(kPool);
  for (std::uint32_t i = 0; i < kPool; ++i) {
    pool.push_back(std::to_string(10000000U + rng.next() % 90000000U));
  }
  Input in;
  std::vector<std::string_view> users;
  users.reserve(lines);
  for (std::size_t i = 0; i < lines; ++i) {
    const std::string_view method = kMethods[rng.next() % 4];
    const unsigned version = 1 + rng.next() % 3;
    const std::string& user = pool[rng.next() % kPool];
    users.push_back(user);
    const std::string before = random_digits(rng);
    const std::string after = random_digits(rng);
    const unsigned status = kStatuses[rng.next() % 12];
    const unsigned ms = rng.next() % 1000;
    in.text += std::format("{} /api/v{}/cart?sid={}{}{} {} {}ms\n", method, version, before, user,
                           after, status, ms);
  }
  // The words: the ids of the log's lines in line order, repeats skipped, the first k.
  std::unordered_set<std::string_view> seen;
  for (const std::string_view w : users) {
    if (in.words.size() == k) {
      break;
    }
    if (seen.insert(w).second) {
      in.words.emplace_back(w);
    }
  }
  return in;
}

Found find_loop(const Input& in) {
  Found f;
  const std::string_view text = in.text;
  for (const std::string& w : in.words) {
    for (std::size_t i = text.find(w); i != std::string_view::npos; i = text.find(w, i + 1)) {
      ++f.matches;
      f.offsets += i;
    }
  }
  return f;
}

Found regex_alternation(const Input& in) {
  std::string pattern;
  for (const std::string& w : in.words) {
    if (!pattern.empty()) {
      pattern += '|';
    }
    pattern += w;
  }
  const std::regex re{pattern};
  Found f;
  for (auto it = std::sregex_iterator(in.text.begin(), in.text.end(), re);
       it != std::sregex_iterator(); ++it) {
    ++f.matches;
    f.offsets += static_cast<std::uint64_t>(it->position());
  }
  return f;
}

// A dense table of 256 columns per state with the failure links folded in (a DFA). out[s] is the
// length of the word that ends in state s, 0 when none does.
Found hand_aho_corasick(const Input& in) {
  std::vector<std::int32_t> next(256, -1);
  std::vector<std::int32_t> out(1, 0);
  for (const std::string& w : in.words) {
    std::size_t s = 0;
    for (const unsigned char c : w) {
      if (next[s * 256 + c] < 0) {
        next[s * 256 + c] = static_cast<std::int32_t>(out.size());
        out.push_back(0);
        next.resize(next.size() + 256, -1);
      }
      s = static_cast<std::size_t>(next[s * 256 + c]);
    }
    out[s] = static_cast<std::int32_t>(w.size());
  }
  std::vector<std::int32_t> fail(out.size(), 0);
  std::queue<std::int32_t> queue;
  for (std::size_t c = 0; c < 256; ++c) {
    if (next[c] < 0) {
      next[c] = 0;
    } else {
      queue.push(next[c]);
    }
  }
  while (!queue.empty()) {
    const auto s = static_cast<std::size_t>(queue.front());
    queue.pop();
    const auto f = static_cast<std::size_t>(fail[s]);
    if (out[s] == 0) {
      out[s] = out[f];
    }
    for (std::size_t c = 0; c < 256; ++c) {
      const std::int32_t t = next[s * 256 + c];
      if (t < 0) {
        next[s * 256 + c] = next[f * 256 + c];
      } else {
        fail[static_cast<std::size_t>(t)] = next[f * 256 + c];
        queue.push(t);
      }
    }
  }
  Found found;
  std::size_t s = 0;
  for (std::size_t i = 0; i < in.text.size(); ++i) {
    s = static_cast<std::size_t>(next[s * 256 + static_cast<unsigned char>(in.text[i])]);
    if (out[s] > 0) {
      ++found.matches;
      found.offsets += i + 1 - static_cast<std::size_t>(out[s]);
    }
  }
  return found;
}

template <std::size_t K, Found (*Way)(const Input&)>
Outcome run_way(const Args& args) {
  const Input in = words_input(args.size, K);
  auto [f, times] = measure(args.repeats, [&] { return Way(in); });
  return {std::format("{}:{}:{}", K, f.matches, f.offsets), std::move(times), {}};
}

template <std::size_t K>
constexpr std::array<Kernel, 3> entries(std::string_view find, std::string_view regex,
                                        std::string_view hand) {
  return {Kernel{find, run_way<K, find_loop>}, Kernel{regex, run_way<K, regex_alternation>},
          Kernel{hand, run_way<K, hand_aho_corasick>}};
}

}  // namespace

std::span<const Kernel> words_kernels() {
  static const auto table = [] {
    std::vector<Kernel> all;
    for (const auto& group :
         {entries<1>("bwords-1.cpp.std.string_view.find", "bwords-1.cpp.std.regex",
                     "bwords-1.cpp.hand.aho-corasick"),
          entries<10>("bwords-10.cpp.std.string_view.find", "bwords-10.cpp.std.regex",
                      "bwords-10.cpp.hand.aho-corasick"),
          entries<100>("bwords-100.cpp.std.string_view.find", "bwords-100.cpp.std.regex",
                       "bwords-100.cpp.hand.aho-corasick"),
          entries<1000>("bwords-1000.cpp.std.string_view.find", "bwords-1000.cpp.std.regex",
                        "bwords-1000.cpp.hand.aho-corasick")}) {
      all.insert(all.end(), group.begin(), group.end());
    }
    return all;
  }();
  return table;
}

}  // namespace kernels
