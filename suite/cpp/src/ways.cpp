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

// libc++ ships std::execution::par only as an experimental library (-fexperimental-library);
// libstdc++ and MSVC's STL declare it whenever <execution> exists.
#if defined(_LIBCPP_VERSION)
#define KERNELS_HAS_EXECUTION_PAR _LIBCPP_HAS_EXPERIMENTAL_PSTL
#elif defined(__cpp_lib_execution)
#define KERNELS_HAS_EXECUTION_PAR 1
#else
#define KERNELS_HAS_EXECUTION_PAR 0
#endif

namespace kernels {
namespace {

constexpr std::string_view kPdqsort = "pdqsort b1ef26a";
constexpr std::string_view kBoostSort = "Boost.Sort 1.92.0";
constexpr std::string_view kDense = "ankerl::unordered_dense 5.3.3";
constexpr std::string_view kBoostUnordered = "Boost.Unordered 1.92.0";

struct WayName {
  std::string_view way;
  std::string_view library;
};

constexpr std::array kSortWays{
    WayName{"std.sort", {}},
    WayName{"std.stable_sort", {}},
    WayName{"std.ranges.sort", {}},
    WayName{"std.sort.par", {}},
    WayName{"pdqsort", kPdqsort},
    WayName{"boost.sort.spreadsort", kBoostSort},
    WayName{"boost.sort.block_indirect_sort", kBoostSort},
};
constexpr std::array kMapWays{
    WayName{"std.unordered_map", {}},
    WayName{"ankerl.unordered_dense.map", kDense},
    WayName{"boost.unordered_flat_map", kBoostUnordered},
};
constexpr std::array kJoinWays{
    WayName{"std.string.append", {}},
    WayName{"std.string.reserve", {}},
    WayName{"std.ostringstream", {}},
    WayName{"std.views.join_with", {}},
};
constexpr std::array<std::string_view, 6> kSortJobs{
    "bsort-int", "bsort-int-sorted", "bsort-int-reversed",
    "bsort-str", "bsort-str-sorted", "bsort-str-reversed",
};
constexpr std::string_view kLangToken = ".cpp.";

Outcome run_way(const Args& args) {
  const std::string_view name = args.kernel;
  const std::size_t at = name.find(kLangToken);
  const std::string_view job = name.substr(0, at);
  const std::string_view way = name.substr(at + kLangToken.size());
  if (job.starts_with("bsort-int")) {
    return ways::sort_ints(args, job, way);
  }
  if (job.starts_with("bsort-str")) {
    return ways::sort_strings(args, job, way);
  }
  if (job == "bmap-int") {
    return ways::map_ints(args, way);
  }
  if (job == "bmap-str") {
    return ways::map_strings(args, way);
  }
  return ways::join(args, way);
}

struct Table {
  std::vector<std::string> names;
  std::vector<Kernel> kernels;
};

Table build_table() {
  Table t;
  std::vector<std::string_view> libraries;
  const auto add = [&](std::string_view job, const auto& way_names) {
    for (const WayName& w : way_names) {
      t.names.push_back(std::format("{}{}{}", job, kLangToken, w.way));
      libraries.push_back(w.library);
    }
  };
  for (const std::string_view job : kSortJobs) {
    add(job, kSortWays);
  }
  add("bmap-int", kMapWays);
  add("bmap-str", kMapWays);
  add("bjoin", kJoinWays);
  // Built after every name is in place, so the string_views into `names` stay valid.
  for (std::size_t i = 0; i < t.names.size(); ++i) {
    t.kernels.push_back(Kernel{t.names[i], run_way, libraries[i]});
  }
  return t;
}

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

#if KERNELS_HAS_EXECUTION_PAR
thread_local bool tls_seen_by_probe = false;

// How many threads std::sort(std::execution::par) uses on a 1M-element sort, counted by a
// comparator that records each new thread once. Run once before timing, so a library whose
// "parallel" backend is sequential (libstdc++ without oneTBB) cannot pass as parallel.
std::size_t execution_par_probe_threads() {
  constexpr std::size_t kProbeSize = 1'000'000;
  std::vector<std::int64_t> probe = ints(kProbeSize);
  std::mutex lock;
  std::set<std::thread::id> seen;
  std::sort(std::execution::par, probe.begin(), probe.end(), [&](std::int64_t x, std::int64_t y) {
    if (!tls_seen_by_probe) {
      tls_seen_by_probe = true;
      const std::scoped_lock guard(lock);
      seen.insert(std::this_thread::get_id());
    }
    return x < y;
  });
  return seen.size();
}

// The pool is the standard library's own (MSVC: the Windows thread pool; libc++: libdispatch),
// sized by the machine, not by --threads.
std::string execution_par_extra() {
  const std::size_t used = execution_par_probe_threads();
  if (used < 2) {
    throw std::runtime_error(
        std::format("std::execution::par ran a 1M-element sort on {} thread: its backend is "
                    "sequential here (libstdc++ needs oneTBB), so this way would not be parallel",
                    used));
  }
  return std::format(R"({{"pool_threads":{},"probe_threads":{}}})",
                     std::thread::hardware_concurrency(), used);
}
#endif

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

}  // namespace

namespace ways {

Outcome sort_ints(const Args& args, std::string_view job, std::string_view way) {
  Sorter<std::int64_t> s = sorter<std::int64_t>(way, args.threads);
  const std::vector<std::int64_t> input = arrange(ints(args.size), job);
  auto [sorted, times] = measure_copy(args.repeats, input, s.sort);
  return {std::to_string(h31(sorted)), std::move(times), std::move(s.extra)};
}

Outcome sort_strings(const Args& args, std::string_view job, std::string_view way) {
  Sorter<std::string> s = sorter<std::string>(way, args.threads);
  const std::vector<std::string> input = arrange(strings(args.size), job);
  auto [sorted, times] = measure_copy(args.repeats, input, s.sort);
  std::string text;
  for (const std::string& v : sorted) {
    text += v;
    text += '\n';
  }
  return {std::to_string(fnv1a(text)), std::move(times), std::move(s.extra)};
}

}  // namespace ways

std::span<const Kernel> ways_kernels() {
  static const Table table = build_table();
  return table.kernels;
}

}  // namespace kernels
