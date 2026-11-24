// Race: sort-int (cpp)
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

void std_sort(std::vector<std::int64_t>& a) { std::ranges::sort(a); }

}  // namespace kernels
