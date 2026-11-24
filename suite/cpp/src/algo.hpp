// Classic algorithm kernels (../SPEC.md section 12, group `algo`), split over three files.
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "kernels.hpp"

namespace kernels {

using Words = std::vector<std::vector<std::uint8_t>>;

// The trie input: N words (3..10 letters), then N prefixes (1..4 letters).
std::pair<Words, Words> words(std::size_t n);  // algo_tree.cpp

Outcome bfs_grid_kernel(const Args& args);     // algo_graph.cpp
Outcome dijkstra_kernel(const Args& args);     // algo_graph.cpp
Outcome kruskal_kernel(const Args& args);      // algo_graph.cpp
Outcome bst_kernel(const Args& args);          // algo_tree.cpp
Outcome ordered_map_kernel(const Args& args);  // algo_tree.cpp
Outcome trie_kernel(const Args& args);         // algo_tree.cpp
Outcome lru_kernel(const Args& args);          // algo_tree.cpp

}  // namespace kernels
