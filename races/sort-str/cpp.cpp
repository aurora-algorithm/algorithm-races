// Race: sort-str (cpp)
// Extracted from suite/cpp/src/sorts.cpp — build and run via suite/run.py

// Sorting kernels, written the same way in every language (../SPEC.md section 3).
#include "sorts.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

Outcome sort_strings(const Args& args) {
  const auto input = strings(args.size);
  auto [sorted, times] =
      measure_copy(args.repeats, input, [](std::vector<std::string>& a) { std::ranges::sort(a); });
  std::string text;
  for (const auto& s : sorted) {
    text += s;
    text += '\n';
  }
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

}  // namespace kernels
