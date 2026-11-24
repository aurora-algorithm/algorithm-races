// Race: fannkuch (cpp)
// Extracted from suite/cpp/src/bg.cpp — build and run via suite/run.py

// Benchmarks Game kernels, single-threaded (../SPEC.md section 4). Floating-point code keeps the
// operation order of the spec so every language prints the same digits.
#include "bg.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <numbers>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

std::string fannkuch(int n) {
  const auto size = static_cast<std::size_t>(n);
  std::vector<std::size_t> perm1(size);
  for (std::size_t i = 0; i < size; ++i) {
    perm1[i] = i;
  }
  std::vector<std::size_t> count(size);
  std::vector<std::size_t> perm(size);
  std::int64_t max_flips = 0;
  std::int64_t checksum = 0;
  std::uint64_t perm_count = 0;
  std::size_t r = size;
  while (true) {
    while (r != 1) {
      count[r - 1] = r;
      --r;
    }
    std::ranges::copy(perm1, perm.begin());
    std::int64_t flips = 0;
    for (std::size_t k = perm[0]; k != 0; k = perm[0]) {
      std::reverse(perm.begin(), perm.begin() + static_cast<std::ptrdiff_t>(k) + 1);
      ++flips;
    }
    max_flips = std::max(max_flips, flips);
    checksum += perm_count % 2 == 0 ? flips : -flips;
    while (true) {
      if (r == size) {
        return std::format("{}:{}", checksum, max_flips);
      }
      std::rotate(perm1.begin(), perm1.begin() + 1,
                  perm1.begin() + static_cast<std::ptrdiff_t>(r) + 1);
      if (--count[r] > 0) {
        break;
      }
      ++r;
    }
    ++perm_count;
  }
}

Outcome fannkuch_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return fannkuch(static_cast<int>(args.size)); }));
}

}  // namespace kernels
