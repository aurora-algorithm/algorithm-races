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
namespace {

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

// Thread t computes rows t, t + T, t + 2T, ... of the shared bitmap.
std::vector<std::uint8_t> mandelbrot_mt(std::size_t n, std::size_t threads) {
  std::vector<std::uint8_t> bitmap(n * row_bytes(n));
  {
    std::vector<std::jthread> workers;
    for (std::size_t t = 0; t < threads; ++t) {
      workers.emplace_back([&bitmap, n, t, threads] {
        for (std::size_t y = t; y < n; y += threads) {
          mandelbrot_row(n, y, std::span(bitmap).subspan(y * row_bytes(n), row_bytes(n)));
        }
      });
    }
  }
  return bitmap;
}

// Each depth's iterations are split into T equal shares; the main thread sums the checks.
std::string binary_trees_mt(int n, std::size_t threads) {
  return binary_trees_lines(n, [threads](int depth, std::uint64_t iterations) {
    const std::uint64_t share = iterations / threads;
    std::vector<std::uint64_t> checks(threads);
    {
      std::vector<std::jthread> workers;
      for (std::size_t t = 0; t < threads; ++t) {
        workers.emplace_back(
            [&checks, t, depth, share] { checks[t] = depth_checks(depth, share); });
      }
    }
    return std::accumulate(checks.begin(), checks.end(), std::uint64_t{0});
  });
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

Outcome mandelbrot_kernel(const Args& args) {
  auto [bitmap, times] =
      measure(args.repeats, [&] { return mandelbrot_mt(args.size, args.threads); });
  return {std::to_string(fnv1a(bitmap)), std::move(times), {}};
}

Outcome binary_trees_kernel(const Args& args) {
  auto [text, times] = measure(
      args.repeats, [&] { return binary_trees_mt(static_cast<int>(args.size), args.threads); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

constexpr std::array kMtKernels = {
    Kernel{"mergesort-mt", mergesort_kernel},
    Kernel{"mandelbrot-mt", mandelbrot_kernel},
    Kernel{"binary-trees-mt", binary_trees_kernel},
};

}  // namespace

std::span<const Kernel> mt_kernels() { return kMtKernels; }

}  // namespace kernels
