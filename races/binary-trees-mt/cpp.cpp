// Race: binary-trees-mt (cpp)
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

Outcome binary_trees_kernel(const Args& args) {
  auto [text, times] = measure(
      args.repeats, [&] { return binary_trees_mt(static_cast<int>(args.size), args.threads); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

}  // namespace kernels
