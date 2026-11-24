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
namespace {

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

std::string sieve(std::size_t n) {
  std::vector<std::uint8_t> composite(n + 1);
  for (std::size_t i = 2; i * i <= n; ++i) {
    if (!composite[i]) {
      for (std::size_t j = i * i; j <= n; j += i) {
        composite[j] = 1;
      }
    }
  }
  std::size_t primes = 0;
  for (std::size_t i = 2; i <= n; ++i) {
    primes += composite[i] ? 0 : 1;
  }
  return std::to_string(primes);
}

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

Outcome regex_kernel(const Args& args) {
  const auto text = log_text(args.size);
  const std::regex pattern{std::string(kLogPattern)};
  return outcome(measure(args.repeats, [&] { return regex_log(pattern, text); }));
}

Outcome hashmap_kernel(const Args& args) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  return outcome(measure(args.repeats, [&] { return hashmap(inserts, lookups); }));
}

Outcome sieve_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return sieve(args.size); }));
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

constexpr std::array kLibKernels = {
    Kernel{"regex-log", regex_kernel, "std::regex (ECMAScript)"},
    Kernel{"regex-log-alt", regex_ctre_kernel, "CTRE 3.11.0"},
    Kernel{"hashmap", hashmap_kernel, "std::unordered_map"},
    Kernel{"sha256", sha256_kernel, "Botan 3.14.1"},
    Kernel{"sieve", sieve_kernel},
    Kernel{"matmul", matmul_kernel},
    Kernel{"json", json_kernel, "nlohmann/json 3.12.0 (ordered_json)"},
};

}  // namespace

std::span<const Kernel> lib_kernels() { return kLibKernels; }

}  // namespace kernels
