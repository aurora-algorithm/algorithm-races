// Race: bsort-int-sorted.goose.sort (cpp)
// Extracted from suite/cpp/src/ways.cpp — build and run via suite/run.py

// The builtins group's dispatch table and its sorts (std::sort, std::stable_sort,
// std::ranges::sort, std::sort(std::execution::par), pdqsort, Boost.Sort).
#include "ways.hpp"
#include <pdqsort/pdqsort.h>
#include <algorithm>
#include <array>
#include <boost/sort/block_indirect_sort/block_indirect_sort.hpp>
#include <boost/sort/spreadsort/spreadsort.hpp>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <format>
#include <functional>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

// libc++ ships std::execution::par only as an experimental library (-fexperimental-library);
// libstdc++ and MSVC's STL declare it whenever <execution> exists.
#if defined(_LIBCPP_VERSION)
#define KERNELS_HAS_EXECUTION_PAR _LIBCPP_HAS_EXPERIMENTAL_PSTL
#elif defined(__cpp_lib_execution)
#define KERNELS_HAS_EXECUTION_PAR 1
#else
#define KERNELS_HAS_EXECUTION_PAR 0
#endif

// random, already ascending or descending: the input order is the only difference between the
// three sort jobs of one element type.
template <class T>
std::vector<T> arrange(std::vector<T> a, std::string_view job) {
  if (job.ends_with("-sorted")) {
    std::ranges::sort(a);
  } else if (job.ends_with("-reversed")) {
    std::ranges::sort(a, std::greater{});
  }
  return a;
}

template <class T>
struct Sorter {
  std::function<void(std::vector<T>&)> sort;
  std::string extra;
};

template <class T>
Sorter<T> sorter(std::string_view way, std::size_t threads) {
  if (way == "std.sort") {
    return {[](std::vector<T>& a) { std::sort(a.begin(), a.end()); }, {}};
  }
  if (way == "std.stable_sort") {
    return {[](std::vector<T>& a) { std::stable_sort(a.begin(), a.end()); }, {}};
  }
  if (way == "std.ranges.sort") {
    return {[](std::vector<T>& a) { std::ranges::sort(a); }, {}};
  }
  if (way == "std.sort.par") {
#if KERNELS_HAS_EXECUTION_PAR
    return {[](std::vector<T>& a) { std::sort(std::execution::par, a.begin(), a.end()); },
            execution_par_extra()};
#else
    throw std::runtime_error("std::execution::par is not in this standard library build");
#endif
  }
  if (way == "pdqsort") {
    return {[](std::vector<T>& a) { pdqsort(a.begin(), a.end()); }, {}};
  }
  if (way == "boost.sort.spreadsort") {
    return {[](std::vector<T>& a) { boost::sort::spreadsort::spreadsort(a.begin(), a.end()); }, {}};
  }
  if (way == "boost.sort.block_indirect_sort") {
    const auto n = static_cast<std::uint32_t>(threads);
    // It starts its n threads inside the call, so that start is timed.
    return {[n](std::vector<T>& a) { boost::sort::block_indirect_sort(a.begin(), a.end(), n); },
            std::format(R"({{"pool_threads":{}}})", n)};
  }
  throw std::invalid_argument(std::format("unknown sort way {}", way));
}

Outcome sort_ints(const Args& args, std::string_view job, std::string_view way) {
  Sorter<std::int64_t> s = sorter<std::int64_t>(way, args.threads);
  const std::vector<std::int64_t> input = arrange(ints(args.size), job);
  auto [sorted, times] = measure_copy(args.repeats, input, s.sort);
  return {std::to_string(h31(sorted)), std::move(times), std::move(s.extra)};
}

}  // namespace kernels
