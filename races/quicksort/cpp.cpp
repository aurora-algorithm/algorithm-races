// Race: quicksort (cpp)
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

// Hoare partition with the middle element as pivot; inclusive bounds.
// Recurses into the smaller side and loops on the larger one.
void quicksort(std::vector<std::int64_t>& a, std::size_t lo, std::size_t hi) {
  while (lo < hi) {
    const std::int64_t pivot = a[lo + (hi - lo) / 2];
    std::size_t i = lo;
    std::size_t j = hi;
    while (true) {
      while (a[i] < pivot) {
        ++i;
      }
      while (a[j] > pivot) {
        --j;
      }
      if (i >= j) {
        break;
      }
      std::swap(a[i], a[j]);
      ++i;
      --j;
    }
    if (j - lo < hi - j) {
      quicksort(a, lo, j);
      lo = j + 1;
    } else {
      quicksort(a, j + 1, hi);
      hi = j;
    }
  }
}

void run_quicksort(std::vector<std::int64_t>& a) {
  if (a.size() > 1) {
    quicksort(a, 0, a.size() - 1);
  }
}

}  // namespace kernels
