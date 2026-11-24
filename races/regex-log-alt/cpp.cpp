// Race: regex-log-alt (cpp)
// Extracted from suite/cpp/src/regex_ctre.cpp — build and run via suite/run.py

// regex-log-alt: the same search with CTRE, a compile-time regular expression library (../SPEC.md
// section 5).
#include <charconv>
#include <cstdint>
#include <ctre.hpp>
#include <format>
#include <string>

#include "inputs.hpp"
#include "lib.hpp"

namespace kernels {
namespace {

constexpr auto kPattern =
    ctll::fixed_string{R"((GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9]))"};

std::string regex_log(const std::string& text) {
  std::uint64_t matches = 0;
  std::uint64_t ids = 0;
  for (const auto& match : ctre::search_all<kPattern>(text)) {
    const auto id = match.get<2>().to_view();
    std::uint64_t value = 0;
    std::from_chars(id.data(), id.data() + id.size(), value);
    ++matches;
    ids += value;
  }
  return std::format("{}:{}", matches, ids);
}

}  // namespace

Outcome regex_ctre_kernel(const Args& args) {
  const auto text = log_text(args.size);
  return outcome(measure(args.repeats, [&] { return regex_log(text); }));
}

}  // namespace kernels
