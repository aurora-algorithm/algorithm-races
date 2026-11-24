// sudoku: Knuth's Algorithm X with dancing links, in flat int arrays (../SPEC.md section 13).
#include <array>
#include <cstdint>
#include <format>
#include <numeric>
#include <string>
#include <vector>

#include "puzzle.hpp"

namespace kernels {
namespace {

constexpr std::size_t kClues = 26;
constexpr std::size_t kColumns = 324;

using Puzzle = std::array<std::uint8_t, 81>;

// Rows (or columns) of a valid grid, shuffled within and between their bands.
std::array<std::size_t, 9> shuffled_lines(XorShift32& rng) {
  std::array<std::size_t, 3> bands = {0, 1, 2};
  shuffle(rng, bands);
  std::array<std::size_t, 9> lines{};
  std::size_t k = 0;
  for (const std::size_t band : bands) {
    std::array<std::size_t, 3> inner = {0, 1, 2};
    shuffle(rng, inner);
    for (const std::size_t i : inner) {
      lines[k++] = band * 3 + i;
    }
  }
  return lines;
}

std::vector<Puzzle> sudoku_puzzles(std::size_t n) {
  XorShift32 rng;
  std::vector<Puzzle> puzzles(n);
  for (Puzzle& puzzle : puzzles) {
    std::array<std::uint8_t, 9> digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    shuffle(rng, digits);
    const auto rows = shuffled_lines(rng);
    const auto cols = shuffled_lines(rng);
    std::array<std::size_t, 81> cells{};
    std::iota(cells.begin(), cells.end(), std::size_t{0});
    shuffle(rng, cells);
    puzzle = {};
    for (std::size_t i = 0; i < kClues; ++i) {
      const std::size_t cell = cells[i];
      const std::size_t r = rows[cell / 9];
      const std::size_t c = cols[cell % 9];
      puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
    }
  }
  return puzzles;
}

// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4
// nodes per row.
class Dlx {
 public:
  Dlx() {
    const std::size_t size = 1 + kColumns + 729 * 4;
    left_.resize(size);
    right_.resize(size);
    up_.resize(size);
    down_.resize(size);
    col_.resize(size);
    row_.resize(size);
    std::iota(up_.begin(), up_.end(), std::size_t{0});
    std::iota(down_.begin(), down_.end(), std::size_t{0});
    std::iota(col_.begin(), col_.end(), std::size_t{0});
    count_.assign(kColumns + 1, 0);
    for (std::size_t i = 0; i <= kColumns; ++i) {
      left_[i] = (i + kColumns) % (kColumns + 1);
      right_[i] = (i + 1) % (kColumns + 1);
    }
    std::size_t node = kColumns + 1;
    for (std::size_t r = 0; r < 9; ++r) {
      for (std::size_t c = 0; c < 9; ++c) {
        const std::size_t b = (r / 3) * 3 + c / 3;
        for (std::size_t d = 0; d < 9; ++d) {
          add_row(node, (r * 9 + c) * 9 + d,
                  {1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d, 244 + b * 9 + d});
          node += 4;
        }
      }
    }
  }

  // Places a clue: covers its row's first column, then the columns of the other nodes going right.
  void place(std::size_t row, std::size_t depth) {
    const std::size_t node = kColumns + 1 + row * 4;
    cover(col_[node]);
    choose_row(node);
    chosen_[depth] = row;
  }

  void search(std::size_t depth) {
    if (right_[0] == 0) {
      ++solutions_;
      if (solutions_ == 1) {
        first_.assign(chosen_.begin(), chosen_.begin() + static_cast<std::ptrdiff_t>(depth));
      }
      return;
    }
    std::size_t best = right_[0];
    for (std::size_t c = right_[best]; c != 0; c = right_[c]) {
      if (count_[c] < count_[best]) {
        best = c;
      }
    }
    cover(best);
    for (std::size_t r = down_[best]; r != best; r = down_[r]) {
      ++tries_;
      chosen_[depth] = row_[r];
      choose_row(r);
      search(depth + 1);
      unchoose_row(r);
    }
    uncover(best);
  }

  std::uint64_t solutions() const { return solutions_; }
  std::uint64_t tries() const { return tries_; }
  const std::vector<std::size_t>& first() const { return first_; }

 private:
  // Appends the four nodes `first..first + 4` at the bottom of their columns and links them in a
  // ring.
  void add_row(std::size_t first, std::size_t row, const std::array<std::size_t, 4>& columns) {
    for (std::size_t k = 0; k < 4; ++k) {
      const std::size_t node = first + k;
      const std::size_t column = columns[k];
      col_[node] = column;
      row_[node] = row;
      up_[node] = up_[column];
      down_[node] = column;
      down_[up_[column]] = node;
      up_[column] = node;
      ++count_[column];
      left_[node] = node - 1;
      right_[node] = node + 1;
    }
    left_[first] = first + 3;
    right_[first + 3] = first;
  }

  void cover(std::size_t c) {
    right_[left_[c]] = right_[c];
    left_[right_[c]] = left_[c];
    for (std::size_t i = down_[c]; i != c; i = down_[i]) {
      for (std::size_t j = right_[i]; j != i; j = right_[j]) {
        down_[up_[j]] = down_[j];
        up_[down_[j]] = up_[j];
        --count_[col_[j]];
      }
    }
  }

  void uncover(std::size_t c) {
    for (std::size_t i = up_[c]; i != c; i = up_[i]) {
      for (std::size_t j = left_[i]; j != i; j = left_[j]) {
        ++count_[col_[j]];
        down_[up_[j]] = j;
        up_[down_[j]] = j;
      }
    }
    right_[left_[c]] = c;
    left_[right_[c]] = c;
  }

  void choose_row(std::size_t node) {
    for (std::size_t j = right_[node]; j != node; j = right_[j]) {
      cover(col_[j]);
    }
  }

  void unchoose_row(std::size_t node) {
    for (std::size_t j = left_[node]; j != node; j = left_[j]) {
      uncover(col_[j]);
    }
  }

  std::vector<std::size_t> left_, right_, up_, down_, col_, row_;
  std::vector<std::uint32_t> count_;
  std::uint64_t solutions_ = 0;
  std::uint64_t tries_ = 0;
  std::array<std::size_t, 81> chosen_{};
  std::vector<std::size_t> first_;
};

struct SudokuResult {
  std::uint64_t solutions = 0;
  std::uint64_t tries = 0;
  std::vector<std::array<std::int64_t, 81>> firsts;
};

SudokuResult sudoku(const std::vector<Puzzle>& puzzles) {
  SudokuResult result;
  result.firsts.reserve(puzzles.size());
  for (const Puzzle& puzzle : puzzles) {
    Dlx dlx;
    std::size_t depth = 0;
    for (std::size_t cell = 0; cell < 81; ++cell) {
      if (puzzle[cell] > 0) {
        dlx.place(cell * 9 + puzzle[cell] - 1, depth++);
      }
    }
    dlx.search(depth);
    result.solutions += dlx.solutions();
    result.tries += dlx.tries();
    auto& grid = result.firsts.emplace_back();
    grid.fill(0);
    for (const std::size_t row : dlx.first()) {
      grid[row / 9] = static_cast<std::int64_t>(row % 9 + 1);
    }
  }
  return result;
}

}  // namespace

Outcome sudoku_kernel(const Args& args) {
  const auto puzzles = sudoku_puzzles(args.size);
  auto [r, times] = measure(args.repeats, [&] { return sudoku(puzzles); });
  std::uint32_t h = 0;
  for (const auto& grid : r.firsts) {
    h = h31(grid, h);  // h31 over [h, 81 digits]
  }
  return {
      std::format("{}:{}:{}:{}", puzzles.size(), r.solutions, r.tries, h), std::move(times), {}};
}

}  // namespace kernels
