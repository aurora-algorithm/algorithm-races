// The Benchmarks Game pieces that the multi-threaded kernels reuse (../SPEC.md sections 4 and 6).
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>

namespace kernels {

// binary-trees' smallest depth (the Benchmarks Game program's min_depth).
inline constexpr int kMinDepth = 4;

// Builds and checks `iterations` trees of `depth`; returns the sum of their checks.
std::uint64_t depth_checks(int depth, std::uint64_t iterations);

// The Benchmarks Game output; `depth_run(depth, iterations)` builds and checks the trees of one
// depth.
std::string binary_trees_lines(int n,
                               const std::function<std::uint64_t(int, std::uint64_t)>& depth_run);

// One bitmap row: bit x (MSB first) is set when point (x, y) stays in the set for 50 iterations.
void mandelbrot_row(std::size_t n, std::size_t y, std::span<std::uint8_t> row);

constexpr std::size_t row_bytes(std::size_t n) { return (n + 7) / 8; }

}  // namespace kernels
