// Race: matmul (cpp)
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

std::vector<double> matmul(const std::vector<double>& a, const std::vector<double>& b,
                           std::size_t n) {
  std::vector<double> c(n * n);
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t row = i * n;
    for (std::size_t j = 0; j < n; ++j) {
      double s = 0.0;
      for (std::size_t k = 0; k < n; ++k) {
        s += a[row + k] * b[k * n + j];
      }
      c[row + j] = s;
    }
  }
  return c;
}

Outcome matmul_kernel(const Args& args) {
  const std::size_t n = args.size;
  const auto [a, b] = matrices(n);
  auto [c, times] = measure(args.repeats, [&] { return matmul(a, b, n); });
  std::int64_t total = 0;
  std::vector<std::int64_t> values(c.size());
  for (std::size_t i = 0; i < c.size(); ++i) {
    values[i] = static_cast<std::int64_t>(c[i]);
    total += values[i];
  }
  return {std::format("{}:{}", total, h31(values)), std::move(times), {}};
}

}  // namespace kernels
