// Race: radixsort (cpp)
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

// LSD radix sort, 8 bits per pass, for values below 2^32.
void radix_sort(std::vector<std::int64_t>& a) {
  std::vector<std::int64_t> buf(a.size());
  for (const int shift : {0, 8, 16, 24}) {
    std::array<std::size_t, 256> count{};
    for (const std::int64_t v : a) {
      ++count[static_cast<std::size_t>((v >> shift) & 255)];
    }
    std::array<std::size_t, 256> pos{};
    std::size_t total = 0;
    for (std::size_t d = 0; d < 256; ++d) {
      pos[d] = total;
      total += count[d];
    }
    for (const std::int64_t v : a) {
      buf[pos[static_cast<std::size_t>((v >> shift) & 255)]++] = v;
    }
    std::swap(a, buf);
  }
}

}  // namespace kernels
