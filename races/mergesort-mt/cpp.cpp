// Race: mergesort-mt (cpp)
// Extracted from suite/cpp/src/mt.cpp — build and run via suite/run.py

// Multi-threaded kernels (../SPEC.md section 6) on std::jthread, started inside the timed call like
// Rust's scoped threads (starting a thread costs microseconds).
#include <array>
#include <bit>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "bg.hpp"
#include "inputs.hpp"
#include "kernels.hpp"
#include "sorts.hpp"

namespace kernels {

// T threads each mergesort one chunk, then rounds of pairwise merges of neighbouring runs (one
// thread per merge) until one run is left. `threads` must be a power of two.
void mergesort_mt(std::vector<std::int64_t>& a, std::size_t threads) {
  const std::size_t n = a.size();
  std::vector<std::int64_t> aux(n);
  std::vector<std::size_t> bounds(threads + 1);
  for (std::size_t t = 0; t <= threads; ++t) {
    bounds[t] = t * n / threads;
  }
  {
    std::vector<std::jthread> workers;
    for (std::size_t t = 0; t < threads; ++t) {
      workers.emplace_back(
          [&a, &aux, lo = bounds[t], hi = bounds[t + 1]] { merge_sort(a, aux, lo, hi); });
    }
  }
  while (bounds.size() > 2) {
    std::vector<std::size_t> merged;
    for (std::size_t i = 0; i < bounds.size(); i += 2) {
      merged.push_back(bounds[i]);
    }
    {
      std::vector<std::jthread> workers;
      for (std::size_t i = 0; i + 2 < bounds.size(); i += 2) {
        workers.emplace_back([&a, &aux, lo = bounds[i], mid = bounds[i + 1], hi = bounds[i + 2]] {
          merge(a, aux, lo, mid, hi);
        });
      }
    }
    bounds = std::move(merged);
  }
}

Outcome mergesort_kernel(const Args& args) {
  if (!std::has_single_bit(args.threads)) {
    throw std::invalid_argument("--threads must be a power of two");
  }
  const auto input = ints(args.size);
  auto [sorted, times] = measure_copy(
      args.repeats, input, [&](std::vector<std::int64_t>& a) { mergesort_mt(a, args.threads); });
  return {std::to_string(h31(sorted)), std::move(times), {}};
}

}  // namespace kernels
