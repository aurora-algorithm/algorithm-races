// Race: str-find (cpp)
// Extracted from suite/cpp/src/algo_misc.cpp — build and run via suite/run.py

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

constexpr std::string_view kAcgt = "acgt";

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

bool env_is(const char* name, std::string_view value) {
  const char* got =
      std::getenv(name);  // NOLINT(concurrency-mt-unsafe): read once, before any thread
  return got != nullptr && value == got;
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

std::pair<std::string, std::string> text(std::size_t n) {
  auto t = std::move(dna(n, 1)[0]);
  auto p = t.substr(n / 2, kPatternLen);
  return {std::move(t), std::move(p)};
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

}  // namespace kernels
