// The program contract shared by every kernel file (../SPEC.md section 1).
#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace kernels {

struct Args {
  std::string kernel;
  std::size_t size = 0;
  std::size_t repeats = 1;
  std::size_t threads = 1;
  std::filesystem::path data = ".";
};

// `extra` is a JSON object (`{"out_bytes":812345}`) or empty.
struct Outcome {
  std::string checksum;
  std::vector<double> times_ms;
  std::string extra;
};

// One entry of the dispatch table: the kernel's name, its runner, and the libraries it uses (for
// `runtime`).
struct Kernel {
  std::string_view name;
  Outcome (*run)(const Args&);
  std::string_view libraries = {};
};

std::span<const Kernel> sort_kernels();
std::span<const Kernel> bg_kernels();
std::span<const Kernel> lib_kernels();
std::span<const Kernel> mt_kernels();
std::span<const Kernel> algo_kernels();
std::span<const Kernel> puzzle_kernels();
std::span<const Kernel> work_kernels();
std::span<const Kernel> arena_kernels();
std::span<const Kernel> ways_kernels();
std::span<const Kernel> words_kernels();
std::span<const Kernel> ssearch_kernels();
std::span<const Kernel> autocomplete_kernels();

inline double elapsed_ms(std::chrono::steady_clock::time_point start) {
  const std::chrono::duration<double, std::milli> ms = std::chrono::steady_clock::now() - start;
  return std::round(ms.count() * 1000.0) / 1000.0;
}

// Runs `run()` once per repeat; returns the last repeat's result and every repeat's time.
template <class Run>
auto measure(std::size_t repeats, Run run) {
  using Result = std::invoke_result_t<Run&>;
  std::optional<Result> last;
  std::vector<double> times;
  times.reserve(repeats);
  for (std::size_t r = 0; r < repeats; ++r) {
    last.reset();
    const auto start = std::chrono::steady_clock::now();
    Result result = run();
    times.push_back(elapsed_ms(start));
    last.emplace(std::move(result));
  }
  return std::pair{std::move(*last), std::move(times)};
}

// Copies `input` before each repeat (outside the timed region) and runs `run(copy)` on it. Returns
// the last result, or the last mutated copy when `run` returns nothing (the sorts).
template <class T, class Run>
auto measure_copy(std::size_t repeats, const T& input, Run run) {
  using Result = std::invoke_result_t<Run&, T&>;
  std::vector<double> times;
  times.reserve(repeats);
  T work;
  if constexpr (std::is_void_v<Result>) {
    for (std::size_t r = 0; r < repeats; ++r) {
      work = input;
      const auto start = std::chrono::steady_clock::now();
      run(work);
      times.push_back(elapsed_ms(start));
    }
    return std::pair{std::move(work), std::move(times)};
  } else {
    std::optional<Result> last;
    for (std::size_t r = 0; r < repeats; ++r) {
      work = input;
      last.reset();
      const auto start = std::chrono::steady_clock::now();
      Result result = run(work);
      times.push_back(elapsed_ms(start));
      last.emplace(std::move(result));
    }
    return std::pair{std::move(*last), std::move(times)};
  }
}

template <class R>
Outcome outcome(std::pair<R, std::vector<double>> measured) {
  return {std::string(std::move(measured.first)), std::move(measured.second), {}};
}

}  // namespace kernels
