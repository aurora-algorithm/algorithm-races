// pi-digits: the Chudnovsky series summed by binary splitting (../SPEC.md section 13). The standard
// library has no big integer; Boost.Multiprecision's cpp_int is the usual header-only choice (GMP
// is ruled out by the spec). The square root is hand-written as in every other language (CPython's
// isqrt).
#include <bit>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <tuple>

#include "puzzle.hpp"

namespace kernels {
namespace {

using boost::multiprecision::cpp_int;

constexpr std::uint64_t kChudA = 13591409;
constexpr std::uint64_t kChudB = 545140134;
constexpr std::uint64_t kChudC3Over24 = 10939058860032000;
constexpr std::size_t kDigitsPerTerm = 14;
constexpr unsigned kGuardDigits = 10;

// P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
std::tuple<cpp_int, cpp_int, cpp_int> binary_split(std::uint64_t a, std::uint64_t b) {
  if (b - a == 1) {
    cpp_int p = 1;
    cpp_int q = 1;
    if (a > 0) {
      p = cpp_int(6 * a - 5) * (2 * a - 1) * (6 * a - 1);
      q = cpp_int(a) * a * a * kChudC3Over24;
    }
    cpp_int t = p * (kChudA + kChudB * a);
    if (a % 2 == 1) {
      t = -t;
    }
    return {std::move(p), std::move(q), std::move(t)};
  }
  const std::uint64_t m = (a + b) / 2;
  auto [p1, q1, t1] = binary_split(a, m);
  auto [p2, q2, t2] = binary_split(m, b);
  return {p1 * p2, q1 * q2, t1 * q2 + p1 * t2};
}

// Floor square root by precision doubling, CPython's math.isqrt algorithm: each round doubles the
// correct bits of `a` with one division, so the work is a few big divisions instead of a Newton
// iteration from scratch.
cpp_int isqrt(const cpp_int& n) {
  const std::uint64_t c = (boost::multiprecision::msb(n)) / 2;  // (bit_length - 1) / 2
  const int rounds = c == 0 ? 0 : static_cast<int>(std::bit_width(c));
  cpp_int a = 1;
  std::uint64_t d = 0;
  for (int s = rounds - 1; s >= 0; --s) {
    const std::uint64_t e = d;
    d = c >> s;
    a = (a << (d - e - 1)) + (n >> (2 * c - e - d + 1)) / a;
  }
  return a * a > n ? a - 1 : a;
}

std::string pi_digits(std::size_t n) {
  const std::uint64_t terms = n / kDigitsPerTerm + 2;
  const auto [p, q, t] = binary_split(0, terms);
  const cpp_int one =
      boost::multiprecision::pow(cpp_int(10), static_cast<unsigned>(n) + kGuardDigits);
  const cpp_int sqrt_c = isqrt(cpp_int(10005) * one * one);
  const cpp_int pi = cpp_int(426880) * sqrt_c * q / t;
  return cpp_int(pi / boost::multiprecision::pow(cpp_int(10), kGuardDigits)).str();
}

}  // namespace

Outcome pi_digits_kernel(const Args& args) {
  auto [digits, times] = measure(args.repeats, [&] { return pi_digits(args.size); });
  const std::string_view last =
      std::string_view(digits).substr(digits.size() > 50 ? digits.size() - 50 : 0);
  std::uint64_t sum = 0;
  for (const char d : digits) {
    sum += static_cast<std::uint64_t>(d - '0');
  }
  return {std::format("{}:{}:{}", digits.size() - 1, last, sum), std::move(times), {}};
}

}  // namespace kernels
