// The ssearch kernels (../SPEC.md section 16): lower bound in small sorted arrays, a plain loop
// against binary search, at every size from 4 to 1024. Kernel names are ssearch.cpp.WAY.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {
namespace {

constexpr std::array<std::size_t, 17> kSizes = {4,  6,   8,   12,  16,  24,  32,  48,  64,
                                                96, 128, 192, 256, 384, 512, 768, 1024};

std::size_t linear(std::span<const std::int64_t> a, std::int64_t q) {
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] >= q) {
      return i;
    }
  }
  return a.size();
}

std::size_t linear_count(std::span<const std::int64_t> a, std::int64_t q) {
  std::size_t count = 0;
  for (const std::int64_t x : a) {
    count += static_cast<std::size_t>(x < q);
  }
  return count;
}

std::size_t binary(std::span<const std::int64_t> a, std::int64_t q) {
  std::size_t lo = 0;
  std::size_t hi = a.size();
  while (lo < hi) {
    const std::size_t mid = (lo + hi) / 2;
    if (a[mid] < q) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

std::size_t lower_bound(std::span<const std::int64_t> a, std::int64_t q) {
  return static_cast<std::size_t>(std::lower_bound(a.begin(), a.end(), q) - a.begin());
}

std::size_t branchless(std::span<const std::int64_t> a, std::int64_t q) {
  const std::int64_t* base = a.data();
  std::size_t len = a.size();
  while (len > 1) {
    const std::size_t half = len / 2;
    base = base[half - 1] < q ? base + half : base;
    len -= half;
  }
  return static_cast<std::size_t>(base - a.data()) +
         static_cast<std::size_t>(len == 1 && *base < q);
}

struct Pass {
  std::uint64_t hits = 0;
  std::uint64_t total = 0;
};

template <std::size_t (*Find)(std::span<const std::int64_t>, std::int64_t)>
Pass query_loop(std::span<const std::int64_t> a, std::span<const std::int64_t> queries) {
  Pass p;
  for (const std::int64_t q : queries) {
    const std::size_t lo = Find(a, q);
    p.hits += static_cast<std::uint64_t>(lo < a.size() && a[lo] == q);
    p.total += lo;
  }
  return p;
}

double median(std::vector<double> v) {
  std::ranges::sort(v);
  const std::size_t m = v.size() / 2;
  return v.size() % 2 ? v[m] : (v[m - 1] + v[m]) / 2;
}

template <std::size_t (*Find)(std::span<const std::int64_t>, std::int64_t)>
Outcome sweep(const Args& args) {
  std::vector<double> times(args.repeats, 0.0);
  std::uint64_t hits = 0;
  std::uint64_t total = 0;
  std::string extra = "{";
  for (const std::size_t n : kSizes) {
    XorShift32 rng;
    std::vector<std::int64_t> a(n);
    std::int64_t v = 0;
    for (auto& x : a) {
      v += 1 + rng.next() % 4;
      x = v;
    }
    std::vector<std::int64_t> queries(args.size);
    for (auto& q : queries) {
      q = static_cast<std::int64_t>(rng.next() % static_cast<std::uint32_t>(v + 1));
    }
    std::vector<double> ns(args.repeats);
    Pass p;
    for (std::size_t r = 0; r < args.repeats; ++r) {
      const auto start = std::chrono::steady_clock::now();
      p = query_loop<Find>(a, queries);
      const std::chrono::duration<double, std::milli> ms = std::chrono::steady_clock::now() - start;
      times[r] += ms.count();
      ns[r] = ms.count() * 1e6 / static_cast<double>(args.size);
    }
    hits += p.hits;
    total += p.total;
    extra += std::format("{}\"ns_{}\":{:.3f}", extra.size() > 1 ? "," : "", n, median(ns));
  }
  for (auto& t : times) {
    t = std::round(t * 1000.0) / 1000.0;
  }
  return {std::format("{}:{}", hits, total), std::move(times), extra + "}"};
}

constexpr std::array kSsearchKernels = {
    Kernel{"ssearch.cpp.linear", sweep<linear>},
    Kernel{"ssearch.cpp.linear-count", sweep<linear_count>},
    Kernel{"ssearch.cpp.binary", sweep<binary>},
    Kernel{"ssearch.cpp.std.lower_bound", sweep<lower_bound>, "std::lower_bound"},
    Kernel{"ssearch.cpp.branchless", sweep<branchless>},
};

}  // namespace

std::span<const Kernel> ssearch_kernels() { return kSsearchKernels; }

}  // namespace kernels
