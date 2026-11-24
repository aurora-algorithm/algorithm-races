// Race: wordcount (cpp)
// Extracted from suite/cpp/src/work.cpp — build and run via suite/run.py

// wordcount (std::getline, split on ASCII whitespace, std::unordered_map) and the work dispatch
// table
// (../SPEC.md section 11).
#include "work.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace kernels {

// Transparent hashing: a word that is already counted is found by its string_view, so only a new
// word builds a std::string, as Rust's to_string per new key.
struct WordHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view s) const noexcept {
    return std::hash<std::string_view>{}(s);
  }
};

using Counts = std::unordered_map<std::string, std::uint64_t, WordHash, std::equal_to<>>;

// ASCII whitespace, as Rust's split_whitespace and C's isspace see this text.
bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

void count_words(std::string_view line, Counts& counts) {
  std::size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && is_space(line[i])) {
      ++i;
    }
    const std::size_t start = i;
    while (i < line.size() && !is_space(line[i])) {
      ++i;
    }
    if (i > start) {
      const std::string_view word = line.substr(start, i - start);
      if (const auto it = counts.find(word); it != counts.end()) {
        ++it->second;
      } else {
        counts.emplace(word, 1);
      }
    }
  }
}

Counts wordcount(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error(std::format("cannot open {}", path.string()));
  }
  Counts counts;
  std::string line;
  while (std::getline(file, line)) {
    count_words(line, counts);
  }
  return counts;
}

// "{words}:{distinct}:{top word}:{top count}", ties to the smaller word.
std::string wordcount_checksum(const Counts& counts) {
  std::uint64_t total = 0;
  const std::string* top_word = nullptr;
  std::uint64_t top = 0;
  for (const auto& [word, count] : counts) {
    total += count;
    if (top_word == nullptr || count > top || (count == top && word < *top_word)) {
      top_word = &word;
      top = count;
    }
  }
  return std::format("{}:{}:{}:{}", total, counts.size(), top_word ? *top_word : "", top);
}

Outcome wordcount_kernel(const Args& args) {
  const auto path = args.data / std::format("text-{}.txt", args.size);
  auto [counts, times] = measure(args.repeats, [&] { return wordcount(path); });
  return {wordcount_checksum(counts), std::move(times), {}};
}

}  // namespace kernels
