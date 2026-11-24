// Race: hashmap (cpp)
// Extracted from suite/cpp/src/misc.cpp — build and run via suite/run.py

// Library and loop kernels (../SPEC.md section 5): std::regex, std::unordered_map, sieve, matmul.
#include <array>
#include <charconv>
#include <cstdint>
#include <format>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"
#include "lib.hpp"

namespace kernels {

std::string hashmap(const std::vector<std::int64_t>& inserts,
                    const std::vector<std::int64_t>& lookups) {
  std::unordered_map<std::int64_t, std::int64_t> counts;
  for (const std::int64_t k : inserts) {
    ++counts[k];
  }
  std::size_t hits = 0;
  for (const std::int64_t k : lookups) {
    hits += counts.contains(k) ? 1 : 0;
  }
  return std::format("{}:{}", counts.size(), hits);
}

Outcome hashmap_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  return outcome(measure(args.repeats, [&] { return hashmap(inserts, lookups); }));
}

}  // namespace kernels
