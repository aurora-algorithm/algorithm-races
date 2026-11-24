// Race: bfs-grid (cpp)
// Extracted from suite/cpp/src/algo_graph.cpp — build and run via suite/run.py

// bfs-grid, dijkstra, kruskal (../SPEC.md section 12).
#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "algo.hpp"
#include "inputs.hpp"

namespace kernels {

std::vector<std::uint8_t> grid(std::size_t n) {
  XorShift32 rng;
  std::vector<std::uint8_t> walls(n * n);
  for (auto& w : walls) {
    w = rng.next() % 100 < 30 ? 1 : 0;
  }
  for (const std::size_t cell :
       {std::size_t{0}, std::size_t{1}, n, n * n - n - 1, n * n - 2, n * n - 1}) {
    walls[cell] = 0;
  }
  return walls;
}

std::string bfs_grid(std::size_t n, const std::vector<std::uint8_t>& walls) {
  std::vector<std::int32_t> dist(n * n, -1);
  dist[0] = 0;
  std::queue<std::size_t> queue;
  queue.push(0);
  while (!queue.empty()) {
    const std::size_t cell = queue.front();
    queue.pop();
    const std::int32_t d = dist[cell] + 1;
    const std::size_t row = cell / n;
    const std::size_t col = cell % n;
    const std::pair<std::size_t, bool> neighbours[] = {
        {cell - n, row > 0},
        {cell + n, row < n - 1},
        {cell - 1, col > 0},
        {cell + 1, col < n - 1},
    };
    for (const auto& [next, ok] : neighbours) {
      if (ok && !walls[next] && dist[next] < 0) {
        dist[next] = d;
        queue.push(next);
      }
    }
  }
  std::size_t reached = 0;
  std::int64_t total = 0;
  for (const std::int32_t d : dist) {
    if (d >= 0) {
      ++reached;
      total += d;
    }
  }
  return std::format("{}:{}:{}", reached, dist[n * n - 1], total);
}

Outcome bfs_grid_kernel(const Args& args) {
  const auto walls = grid(args.size);
  return outcome(measure(args.repeats, [&] { return bfs_grid(args.size, walls); }));
}

}  // namespace kernels
