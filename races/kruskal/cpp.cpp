// Race: kruskal (cpp)
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

struct Edge {
  std::uint32_t w;
  std::size_t u;
  std::size_t v;
};

std::vector<Edge> edge_list(std::size_t n) {
  XorShift32 rng;
  std::vector<Edge> edges(4 * n);
  for (Edge& e : edges) {
    e.u = rng.next() % n;
    e.v = rng.next() % n;
    e.w = rng.next() % 1000000;
  }
  return edges;
}

std::size_t find(std::vector<std::size_t>& parent, std::size_t x) {
  std::size_t root = x;
  while (parent[root] != root) {
    root = parent[root];
  }
  while (parent[x] != root) {
    const std::size_t next = parent[x];
    parent[x] = root;
    x = next;
  }
  return root;
}

std::string kruskal(std::size_t n, std::vector<Edge>& edges) {
  std::ranges::sort(edges, {}, &Edge::w);
  std::vector<std::size_t> parent(n);
  std::iota(parent.begin(), parent.end(), std::size_t{0});
  std::vector<std::uint8_t> rank(n);
  std::uint64_t count = 0;
  std::uint64_t total = 0;
  for (const Edge& e : edges) {
    std::size_t ru = find(parent, e.u);
    std::size_t rv = find(parent, e.v);
    if (ru == rv) {
      continue;
    }
    if (rank[ru] < rank[rv]) {
      std::swap(ru, rv);
    }
    parent[rv] = ru;
    if (rank[ru] == rank[rv]) {
      ++rank[ru];
    }
    ++count;
    total += e.w;
  }
  return std::format("{}:{}", count, total);
}

Outcome kruskal_kernel(const Args& args) {
  const auto edges = edge_list(args.size);
  return outcome(measure_copy(args.repeats, edges,
                              [&](std::vector<Edge>& e) { return kruskal(args.size, e); }));
}

}  // namespace kernels
