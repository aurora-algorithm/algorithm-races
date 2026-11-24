// Unusual algorithm kernels (../SPEC.md section 13, group `puzzle`), split over four files.
#pragma once

#include <cstdint>
#include <span>
#include <utility>

#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

Outcome sudoku_kernel(const Args& args);        // sudoku.cpp
Outcome palindrome_kernel(const Args& args);    // puzzle_text.cpp
Outcome bwt_kernel(const Args& args);           // puzzle_text.cpp
Outcome aho_corasick_kernel(const Args& args);  // puzzle_text.cpp
Outcome huffman_kernel(const Args& args);       // puzzle_text.cpp
Outcome pi_digits_kernel(const Args& args);     // pi.cpp

// Fisher-Yates: for i = len-1 down to 1, swap a[i] with a[next() % (i+1)].
template <class Range>
void shuffle(XorShift32& rng, Range& items) {
  for (std::size_t i = items.size(); i-- > 1;) {
    const std::size_t j = rng.next() % (i + 1);
    std::swap(items[i], items[j]);
  }
}

}  // namespace kernels
