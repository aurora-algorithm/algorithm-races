// Race: spectral-norm (cpp)
// Extracted from suite/cpp/src/bg.cpp — build and run via suite/run.py

// Benchmarks Game kernels, single-threaded (../SPEC.md section 4). Floating-point code keeps the
// operation order of the spec so every language prints the same digits.
#include "bg.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <memory>
#include <numbers>
#include <vector>
#include "inputs.hpp"
#include "kernels.hpp"

namespace kernels {

double eval_a(std::size_t i, std::size_t j) {
  return 1.0 / static_cast<double>((i + j) * (i + j + 1) / 2 + i + 1);
}

std::vector<double> mul_av(const std::vector<double>& u) {
  const std::size_t n = u.size();
  std::vector<double> v(n);
  for (std::size_t i = 0; i < n; ++i) {
    double s = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
      s += eval_a(i, j) * u[j];
    }
    v[i] = s;
  }
  return v;
}

std::vector<double> mul_atv(const std::vector<double>& u) {
  const std::size_t n = u.size();
  std::vector<double> v(n);
  for (std::size_t i = 0; i < n; ++i) {
    double s = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
      s += eval_a(j, i) * u[j];
    }
    v[i] = s;
  }
  return v;
}

std::vector<double> mul_atav(const std::vector<double>& u) { return mul_atv(mul_av(u)); }

std::string spectral_norm(std::size_t n) {
  std::vector<double> u(n, 1.0);
  std::vector<double> v;
  for (int i = 0; i < 10; ++i) {
    v = mul_atav(u);
    u = mul_atav(v);
  }
  double vbv = 0.0;
  double vv = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    vbv += u[i] * v[i];
    vv += v[i] * v[i];
  }
  return std::format("{:.9f}", std::sqrt(vbv / vv));
}

Outcome spectral_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return spectral_norm(args.size); }));
}

}  // namespace kernels
