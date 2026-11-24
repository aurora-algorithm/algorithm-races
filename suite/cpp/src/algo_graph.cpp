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
namespace {

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

}  // namespace

Outcome bfs_grid_kernel(const Args& args) {
  const auto walls = grid(args.size);
  return outcome(measure(args.repeats, [&] { return bfs_grid(args.size, walls); }));
}

Outcome dijkstra_kernel(const Args& args) {
  const auto g = graph(args.size);
  return outcome(measure(args.repeats, [&] { return dijkstra(args.size, g); }));
}

Outcome kruskal_kernel(const Args& args) {
  const auto edges = edge_list(args.size);
  return outcome(measure_copy(args.repeats, edges,
                              [&](std::vector<Edge>& e) { return kruskal(args.size, e); }));
}

}  // namespace kernels
