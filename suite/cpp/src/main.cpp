// CPU kernels, C++ version (see ../SPEC.md).
//
// usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
// Prints one JSON line: kernel, size, threads, checksum, times_ms (kernel only), runtime.
#include <charconv>
#include <cstdio>
#include <exception>
#include <expected>
#include <format>
#include <iterator>
#include <print>
#include <string>
#include <string_view>

#include "build_info.hpp"
#include "kernels.hpp"

namespace {

using kernels::Args;
using kernels::Kernel;

std::expected<std::size_t, std::string> positive(std::string_view flag, std::string_view value) {
  std::size_t v = 0;
  const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), v);
  if (ec != std::errc{} || end != value.data() + value.size() || v == 0) {
    return std::unexpected(std::format("{}: expected a positive integer, got {}", flag, value));
  }
  return v;
}

std::expected<Args, std::string> parse_args(int argc, char** argv) {
  if (argc < 2) {
    return std::unexpected("missing KERNEL");
  }
  Args args{.kernel = argv[1]};
  for (int i = 2; i < argc; i += 2) {
    const std::string_view flag = argv[i];
    if (i + 1 >= argc) {
      return std::unexpected(std::format("{} needs a value", flag));
    }
    const std::string_view value = argv[i + 1];
    if (flag == "--data") {
      args.data = value;
      continue;
    }
    auto number = positive(flag, value);
    if (!number) {
      return std::unexpected(number.error());
    }
    if (flag == "--size") {
      args.size = *number;
    } else if (flag == "--repeats") {
      args.repeats = *number;
    } else if (flag == "--threads") {
      args.threads = *number;
    } else {
      return std::unexpected(std::format("unknown option {}", flag));
    }
  }
  if (args.size == 0) {
    return std::unexpected("--size is required");
  }
  return args;
}

const Kernel* find(std::string_view name) {
  for (const auto table :
       {kernels::sort_kernels(), kernels::bg_kernels(), kernels::lib_kernels(),
        kernels::mt_kernels(), kernels::algo_kernels(), kernels::puzzle_kernels(),
        kernels::work_kernels(), kernels::arena_kernels(), kernels::ways_kernels(),
        kernels::ssearch_kernels(), kernels::autocomplete_kernels(), kernels::words_kernels()}) {
    for (const Kernel& k : table) {
      if (k.name == name) {
        return &k;
      }
    }
  }
  return nullptr;
}

std::string compiler() {
#if defined(__clang__) && defined(_MSC_VER)
  std::string cc = std::format("clang {} (MSVC ABI, _MSC_VER {})", __clang_version__, _MSC_VER);
#elif defined(__clang__)
  std::string cc = std::format("clang {}", __clang_version__);
#elif defined(__GNUC__)
  std::string cc = std::format("gcc {}", __VERSION__);
#elif defined(_MSC_VER)
  std::string cc = std::format("msvc {}", _MSC_FULL_VER);
#else
  std::string cc = "unknown compiler";
#endif
#if defined(_LIBCPP_VERSION)
  cc += std::format(", libc++ {}", _LIBCPP_VERSION);
#elif defined(__GLIBCXX__)
  cc += std::format(", libstdc++ {}", __GLIBCXX__);
#elif defined(_MSVC_STL_UPDATE)
  cc += std::format(", MSVC STL {}", _MSVC_STL_UPDATE);
#endif
  return cc;
}

std::string json_string(std::string_view text) {
  std::string out = "\"";
  for (const char c : text) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < 0x20) {
      std::format_to(std::back_inserter(out), "\\u{:04x}", static_cast<unsigned>(c));
    } else {
      out += c;
    }
  }
  return out + '"';
}

std::string row(const Args& args, const Kernel& kernel, const kernels::Outcome& result) {
  std::string runtime = std::format("{} [{}]", compiler(), kernels::kBuildFlags);
  if (!kernel.libraries.empty()) {
    runtime += std::format(" {}", kernel.libraries);
  }
  std::string times;
  for (const double t : result.times_ms) {
    std::format_to(std::back_inserter(times), "{}{}", times.empty() ? "" : ",", t);
  }
  std::string line = std::format(
      R"({{"kernel":{},"size":{},"threads":{},"checksum":{},"times_ms":[{}],"runtime":{})",
      json_string(args.kernel), args.size, args.threads, json_string(result.checksum), times,
      json_string(runtime));
  if (!result.extra.empty()) {
    line += std::format(R"(,"extra":{})", result.extra);
  }
  return line + "}";
}

}  // namespace

int main(int argc, char** argv) {
  const auto args = parse_args(argc, argv);
  if (!args) {
    std::println(stderr, "error: {}", args.error());
    std::println(stderr, "usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]");
    return 2;
  }
  const Kernel* kernel = find(args->kernel);
  if (kernel == nullptr) {
    std::println(stderr, "unknown kernel {}", args->kernel);
    return 2;
  }
  try {
    const auto result = kernel->run(*args);
    std::println("{}", row(*args, *kernel, result));
  } catch (const std::exception& e) {
    std::println(stderr, "error: {}", e.what());
    return 1;
  }
  return 0;
}
