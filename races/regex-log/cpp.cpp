// Race: regex-log (cpp)
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

std::string regex_log(const std::regex& pattern, const std::string& text) {
  std::uint64_t matches = 0;
  std::uint64_t ids = 0;
  for (auto it = std::sregex_iterator(text.begin(), text.end(), pattern);
       it != std::sregex_iterator(); ++it) {
    const std::string_view id((*it)[2].first, (*it)[2].second);
    std::uint64_t value = 0;
    std::from_chars(id.data(), id.data() + id.size(), value);
    ++matches;
    ids += value;
  }
  return std::format("{}:{}", matches, ids);
}

Outcome regex_kernel(const Args& args) {
  const auto text = log_text(args.size);
  const std::regex pattern{std::string(kLogPattern)};
  return outcome(measure(args.repeats, [&] { return regex_log(pattern, text); }));
}

}  // namespace kernels
