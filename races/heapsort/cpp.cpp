// Race: heapsort (cpp)
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

void sift_down(std::vector<std::int64_t>& a, std::size_t root, std::size_t end) {
  while (2 * root + 1 < end) {
    std::size_t child = 2 * root + 1;
    if (child + 1 < end && a[child] < a[child + 1]) {
      ++child;
    }
    if (a[root] < a[child]) {
      std::swap(a[root], a[child]);
      root = child;
    } else {
      return;
    }
  }
}

void heapsort(std::vector<std::int64_t>& a) {
  const std::size_t n = a.size();
  for (std::size_t start = n / 2; start-- > 0;) {
    sift_down(a, start, n);
  }
  for (std::size_t end = n; end-- > 1;) {
    std::swap(a[0], a[end]);
    sift_down(a, 0, end);
  }
}

}  // namespace kernels
