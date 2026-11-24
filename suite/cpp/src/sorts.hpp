// The hand-written sorts that the multi-threaded mergesort reuses (../SPEC.md section 3).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kernels {

// Top-down mergesort of the half-open range [lo, hi) of `a`, using `aux` as scratch space.
void merge_sort(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
                std::size_t hi);

// Merges the sorted runs [lo, mid) and [mid, hi) of `a`; stable.
void merge(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
           std::size_t mid, std::size_t hi);

}  // namespace kernels
