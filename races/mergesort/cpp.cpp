// Race: mergesort (cpp)
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

void run_mergesort(std::vector<std::int64_t>& a) {
  std::vector<std::int64_t> aux(a.size());
  merge_sort(a, aux, 0, a.size());
}

void merge_sort(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
                std::size_t hi) {
  if (hi - lo < 2) {
    return;
  }
  const std::size_t mid = lo + (hi - lo) / 2;
  merge_sort(a, aux, lo, mid);
  merge_sort(a, aux, mid, hi);
  merge(a, aux, lo, mid, hi);
}

void merge(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
           std::size_t mid, std::size_t hi) {
  std::copy(a.data() + lo, a.data() + hi, aux.data() + lo);
  std::size_t i = lo;
  std::size_t j = mid;
  std::size_t k = lo;
  while (i < mid && j < hi) {
    if (aux[j] < aux[i]) {
      a[k++] = aux[j++];
    } else {
      a[k++] = aux[i++];
    }
  }
  while (i < mid) {
    a[k++] = aux[i++];
  }
  while (j < hi) {
    a[k++] = aux[j++];
  }
}

}  // namespace kernels
