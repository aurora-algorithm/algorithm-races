// Race: mandelbrot (cpp)
// Extracted from suite/cpp/src/bg.cpp — build and run via suite/run.py

// Benchmarks Game kernels, single-threaded (../SPEC.md section 4). Floating-point code keeps the
// operation order of the spec so every language prints the same digits.
#include "bg.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <numbers>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

std::vector<std::uint8_t> mandelbrot(std::size_t n) {
  std::vector<std::uint8_t> bitmap(n * row_bytes(n));
  for (std::size_t y = 0; y < n; ++y) {
    mandelbrot_row(n, y, std::span(bitmap).subspan(y * row_bytes(n), row_bytes(n)));
  }
  return bitmap;
}

Outcome mandelbrot_kernel(const Args& args) {
  auto [bitmap, times] = measure(args.repeats, [&] { return mandelbrot(args.size); });
  return {std::to_string(fnv1a(bitmap)), std::move(times), {}};
}

void mandelbrot_row(std::size_t n, std::size_t y, std::span<std::uint8_t> row) {
  const double size = static_cast<double>(n);
  const double ci = 2.0 * static_cast<double>(y) / size - 1.0;
  for (std::size_t x = 0; x < n; ++x) {
    const double cr = 2.0 * static_cast<double>(x) / size - 1.5;
    double zr = 0.0;
    double zi = 0.0;
    double tr = 0.0;
    double ti = 0.0;
    for (int i = 0; i < 50 && tr + ti <= 4.0; ++i) {
      zi = 2.0 * zr * zi + ci;
      zr = tr - ti + cr;
      tr = zr * zr;
      ti = zi * zi;
    }
    if (tr + ti <= 4.0) {
      row[x >> 3] |= static_cast<std::uint8_t>(0x80 >> (x & 7));
    }
  }
}

}  // namespace kernels
