// Race: dijkstra (cpp)
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

struct Graph {
  std::vector<std::size_t> to;
  std::vector<std::uint64_t> weight;
};

// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
Graph graph(std::size_t n) {
  XorShift32 rng;
  Graph g{std::vector<std::size_t>(4 * n), std::vector<std::uint64_t>(4 * n)};
  for (std::size_t u = 0; u < n; ++u) {
    g.to[4 * u] = (u + 1) % n;
    g.weight[4 * u] = 1 + rng.next() % 100;
    for (std::size_t e = 4 * u + 1; e < 4 * u + 4; ++e) {
      g.to[e] = rng.next() % n;
      g.weight[e] = 1 + rng.next() % 100;
    }
  }
  return g;
}

std::string dijkstra(std::size_t n, const Graph& g) {
  constexpr auto kUnseen = std::numeric_limits<std::uint64_t>::max();
  std::vector<std::uint64_t> dist(n, kUnseen);
  dist[0] = 0;
  using Item = std::pair<std::uint64_t, std::size_t>;
  const auto farther = [](const Item& a, const Item& b) { return a.first > b.first; };
  std::priority_queue<Item, std::vector<Item>, decltype(farther)> heap(farther);
  heap.emplace(0, 0);
  while (!heap.empty()) {
    const auto [d, u] = heap.top();
    heap.pop();
    if (d > dist[u]) {
      continue;
    }
    for (std::size_t e = 4 * u; e < 4 * u + 4; ++e) {
      const std::size_t v = g.to[e];
      const std::uint64_t nd = d + g.weight[e];
      if (nd < dist[v]) {
        dist[v] = nd;
        heap.emplace(nd, v);
      }
    }
  }
  std::size_t reached = 0;
  std::uint64_t total = 0;
  std::uint64_t longest = 0;
  for (const std::uint64_t d : dist) {
    if (d < kUnseen) {
      ++reached;
      total += d;
      longest = std::max(longest, d);
    }
  }
  return std::format("{}:{}:{}", reached, total, longest);
}

Outcome dijkstra_kernel(const Args& args) {
  const auto g = graph(args.size);
  return outcome(measure(args.repeats, [&] { return dijkstra(args.size, g); }));
}

}  // namespace kernels
