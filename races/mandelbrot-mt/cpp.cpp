// Race: mandelbrot-mt (cpp)
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

Outcome mandelbrot_kernel(const Args& args) {
  auto [bitmap, times] =
      measure(args.repeats, [&] { return mandelbrot_mt(args.size, args.threads); });
  return {std::to_string(fnv1a(bitmap)), std::move(times), {}};
}

}  // namespace kernels
