// Race: sieve (cpp)
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

Outcome sieve_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return sieve(args.size); }));
}

}  // namespace kernels
