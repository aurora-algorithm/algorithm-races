// Sorting kernels, written the same way in every language (../SPEC.md section 3).
#include "sorts.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {
namespace {

// Hoare partition with the middle element as pivot; inclusive bounds.
// Recurses into the smaller side and loops on the larger one.
void quicksort(std::vector<std::int64_t>& a, std::size_t lo, std::size_t hi) {
  while (lo < hi) {
    const std::int64_t pivot = a[lo + (hi - lo) / 2];
    std::size_t i = lo;
    std::size_t j = hi;
    while (true) {
      while (a[i] < pivot) {
        ++i;
      }
      while (a[j] > pivot) {
        --j;
      }
      if (i >= j) {
        break;
      }
      std::swap(a[i], a[j]);
      ++i;
      --j;
    }
    if (j - lo < hi - j) {
      quicksort(a, lo, j);
      lo = j + 1;
    } else {
      quicksort(a, j + 1, hi);
      hi = j;
    }
  }
}

void sift_down(std::vector<std::int64_t>& a, std::size_t root, std::size_t end) {
  while (2 * root + 1 < end) {
    std::size_t child = 2 * root + 1;
    if (child + 1 < end && a[child] < a[child + 1]) {
      ++child;
    }
    if (a[root] < a[child]) {
      std::swap(a[root], a[child]);
      root = child;
    } else {
      return;
    }
  }
}

void heapsort(std::vector<std::int64_t>& a) {
  const std::size_t n = a.size();
  for (std::size_t start = n / 2; start-- > 0;) {
    sift_down(a, start, n);
  }
  for (std::size_t end = n; end-- > 1;) {
    std::swap(a[0], a[end]);
    sift_down(a, 0, end);
  }
}

// LSD radix sort, 8 bits per pass, for values below 2^32.
void radix_sort(std::vector<std::int64_t>& a) {
  std::vector<std::int64_t> buf(a.size());
  for (const int shift : {0, 8, 16, 24}) {
    std::array<std::size_t, 256> count{};
    for (const std::int64_t v : a) {
      ++count[static_cast<std::size_t>((v >> shift) & 255)];
    }
    std::array<std::size_t, 256> pos{};
    std::size_t total = 0;
    for (std::size_t d = 0; d < 256; ++d) {
      pos[d] = total;
      total += count[d];
    }
    for (const std::int64_t v : a) {
      buf[pos[static_cast<std::size_t>((v >> shift) & 255)]++] = v;
    }
    std::swap(a, buf);
  }
}

std::string sorted_checksum(const std::vector<std::int64_t>& a) { return std::to_string(h31(a)); }

template <void (*Sort)(std::vector<std::int64_t>&)>
Outcome sort_kernel(const Args& args) {
  const auto input = ints(args.size);
  auto [sorted, times] = measure_copy(args.repeats, input, Sort);
  return {sorted_checksum(sorted), std::move(times), {}};
}

void std_sort(std::vector<std::int64_t>& a) { std::ranges::sort(a); }

void run_quicksort(std::vector<std::int64_t>& a) {
  if (a.size() > 1) {
    quicksort(a, 0, a.size() - 1);
  }
}

void run_mergesort(std::vector<std::int64_t>& a) {
  std::vector<std::int64_t> aux(a.size());
  merge_sort(a, aux, 0, a.size());
}

Outcome sort_strings(const Args& args) {
  const auto input = strings(args.size);
  auto [sorted, times] =
      measure_copy(args.repeats, input, [](std::vector<std::string>& a) { std::ranges::sort(a); });
  std::string text;
  for (const auto& s : sorted) {
    text += s;
    text += '\n';
  }
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

constexpr std::array kSortKernels = {
    Kernel{"sort-int", sort_kernel<std_sort>},
    Kernel{"sort-str", sort_strings},
    Kernel{"quicksort", sort_kernel<run_quicksort>},
    Kernel{"mergesort", sort_kernel<run_mergesort>},
    Kernel{"heapsort", sort_kernel<heapsort>},
    Kernel{"radixsort", sort_kernel<radix_sort>},
};

}  // namespace

void merge_sort(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
                std::size_t hi) {
  if (hi - lo < 2) {
    return;
  }
  const std::size_t mid = lo + (hi - lo) / 2;
  merge_sort(a, aux, lo, mid);
  merge_sort(a, aux, mid, hi);
  merge(a, aux, lo, mid, hi);
}

void merge(std::vector<std::int64_t>& a, std::vector<std::int64_t>& aux, std::size_t lo,
           std::size_t mid, std::size_t hi) {
  std::copy(a.data() + lo, a.data() + hi, aux.data() + lo);
  std::size_t i = lo;
  std::size_t j = mid;
  std::size_t k = lo;
  while (i < mid && j < hi) {
    if (aux[j] < aux[i]) {
      a[k++] = aux[j++];
    } else {
      a[k++] = aux[i++];
    }
  }
  while (i < mid) {
    a[k++] = aux[i++];
  }
  while (j < hi) {
    a[k++] = aux[j++];
  }
}

std::span<const Kernel> sort_kernels() { return kSortKernels; }

}  // namespace kernels
