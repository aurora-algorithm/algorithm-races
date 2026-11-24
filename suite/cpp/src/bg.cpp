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
namespace {

struct TreeNode {
  std::unique_ptr<TreeNode> left;
  std::unique_ptr<TreeNode> right;
};

std::unique_ptr<TreeNode> make_tree(int depth) {
  auto node = std::make_unique<TreeNode>();
  if (depth > 0) {
    node->left = make_tree(depth - 1);
    node->right = make_tree(depth - 1);
  }
  return node;
}

std::uint64_t check_tree(const TreeNode& node) {
  if (node.left && node.right) {
    return 1 + check_tree(*node.left) + check_tree(*node.right);
  }
  return 1;
}

constexpr double kSolarMass = 4.0 * std::numbers::pi * std::numbers::pi;
constexpr double kDaysPerYear = 365.24;

struct Body {
  double x, y, z, vx, vy, vz, mass;
};

// The Benchmarks Game's literals, kept digit for digit (they round to the same doubles).
constexpr std::array<Body, 5> kBodies = {{
    // Sun
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, kSolarMass},
    // Jupiter
    {4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01,
     1.66007664274403694e-03 * kDaysPerYear, 7.69901118419740425e-03 * kDaysPerYear,
     -6.90460016972063023e-05 * kDaysPerYear, 9.54791938424326609e-04 * kSolarMass},
    // Saturn
    {8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01,
     -2.76742510726862411e-03 * kDaysPerYear, 4.99852801234917238e-03 * kDaysPerYear,
     2.30417297573763929e-05 * kDaysPerYear, 2.85885980666130812e-04 * kSolarMass},
    // Uranus
    {1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01,
     2.96460137564761618e-03 * kDaysPerYear, 2.37847173959480950e-03 * kDaysPerYear,
     -2.96589568540237556e-05 * kDaysPerYear, 4.36624404335156298e-05 * kSolarMass},
    // Neptune
    {1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01,
     2.68067772490389322e-03 * kDaysPerYear, 1.62824170038242295e-03 * kDaysPerYear,
     -9.51592254519715870e-05 * kDaysPerYear, 5.15138902046611451e-05 * kSolarMass},
}};

void offset_momentum(std::array<Body, 5>& bodies) {
  double px = 0.0;
  double py = 0.0;
  double pz = 0.0;
  for (const Body& b : bodies) {
    px += b.vx * b.mass;
    py += b.vy * b.mass;
    pz += b.vz * b.mass;
  }
  bodies[0].vx = -px / kSolarMass;
  bodies[0].vy = -py / kSolarMass;
  bodies[0].vz = -pz / kSolarMass;
}

double energy(const std::array<Body, 5>& bodies) {
  double e = 0.0;
  for (std::size_t i = 0; i < bodies.size(); ++i) {
    const Body& bi = bodies[i];
    e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz);
    for (std::size_t j = i + 1; j < bodies.size(); ++j) {
      const Body& bj = bodies[j];
      const double dx = bi.x - bj.x;
      const double dy = bi.y - bj.y;
      const double dz = bi.z - bj.z;
      e -= (bi.mass * bj.mass) / std::sqrt(dx * dx + dy * dy + dz * dz);
    }
  }
  return e;
}

void advance(std::array<Body, 5>& bodies, double dt, std::size_t steps) {
  for (std::size_t step = 0; step < steps; ++step) {
    for (std::size_t i = 0; i < bodies.size(); ++i) {
      Body& bi = bodies[i];
      for (std::size_t j = i + 1; j < bodies.size(); ++j) {
        Body& bj = bodies[j];
        const double dx = bi.x - bj.x;
        const double dy = bi.y - bj.y;
        const double dz = bi.z - bj.z;
        const double d2 = dx * dx + dy * dy + dz * dz;
        const double mag = dt / (d2 * std::sqrt(d2));
        const double bim = bi.mass * mag;
        const double bjm = bj.mass * mag;
        bi.vx -= dx * bjm;
        bi.vy -= dy * bjm;
        bi.vz -= dz * bjm;
        bj.vx += dx * bim;
        bj.vy += dy * bim;
        bj.vz += dz * bim;
      }
    }
    for (Body& b : bodies) {
      b.x += dt * b.vx;
      b.y += dt * b.vy;
      b.z += dt * b.vz;
    }
  }
}

std::string nbody(std::size_t steps) {
  auto bodies = kBodies;
  offset_momentum(bodies);
  const double before = energy(bodies);
  advance(bodies, 0.01, steps);
  return std::format("{:.9f};{:.9f}", before, energy(bodies));
}

std::vector<std::uint8_t> mandelbrot(std::size_t n) {
  std::vector<std::uint8_t> bitmap(n * row_bytes(n));
  for (std::size_t y = 0; y < n; ++y) {
    mandelbrot_row(n, y, std::span(bitmap).subspan(y * row_bytes(n), row_bytes(n)));
  }
  return bitmap;
}

std::string fannkuch(int n) {
  const auto size = static_cast<std::size_t>(n);
  std::vector<std::size_t> perm1(size);
  for (std::size_t i = 0; i < size; ++i) {
    perm1[i] = i;
  }
  std::vector<std::size_t> count(size);
  std::vector<std::size_t> perm(size);
  std::int64_t max_flips = 0;
  std::int64_t checksum = 0;
  std::uint64_t perm_count = 0;
  std::size_t r = size;
  while (true) {
    while (r != 1) {
      count[r - 1] = r;
      --r;
    }
    std::ranges::copy(perm1, perm.begin());
    std::int64_t flips = 0;
    for (std::size_t k = perm[0]; k != 0; k = perm[0]) {
      std::reverse(perm.begin(), perm.begin() + static_cast<std::ptrdiff_t>(k) + 1);
      ++flips;
    }
    max_flips = std::max(max_flips, flips);
    checksum += perm_count % 2 == 0 ? flips : -flips;
    while (true) {
      if (r == size) {
        return std::format("{}:{}", checksum, max_flips);
      }
      std::rotate(perm1.begin(), perm1.begin() + 1,
                  perm1.begin() + static_cast<std::ptrdiff_t>(r) + 1);
      if (--count[r] > 0) {
        break;
      }
      ++r;
    }
    ++perm_count;
  }
}

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

Outcome binary_trees_kernel(const Args& args) {
  const int depth = static_cast<int>(args.size);
  auto [text, times] =
      measure(args.repeats, [depth] { return binary_trees_lines(depth, depth_checks); });
  return {std::to_string(fnv1a(text)), std::move(times), {}};
}

Outcome nbody_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return nbody(args.size); }));
}

Outcome mandelbrot_kernel(const Args& args) {
  auto [bitmap, times] = measure(args.repeats, [&] { return mandelbrot(args.size); });
  return {std::to_string(fnv1a(bitmap)), std::move(times), {}};
}

Outcome fannkuch_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return fannkuch(static_cast<int>(args.size)); }));
}

Outcome spectral_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return spectral_norm(args.size); }));
}

constexpr std::array kBgKernels = {
    Kernel{"binary-trees", binary_trees_kernel}, Kernel{"nbody", nbody_kernel},
    Kernel{"mandelbrot", mandelbrot_kernel},     Kernel{"fannkuch", fannkuch_kernel},
    Kernel{"spectral-norm", spectral_kernel},
};

}  // namespace

std::uint64_t depth_checks(int depth, std::uint64_t iterations) {
  std::uint64_t sum = 0;
  for (std::uint64_t i = 0; i < iterations; ++i) {
    sum += check_tree(*make_tree(depth));
  }
  return sum;
}

std::string binary_trees_lines(int n,
                               const std::function<std::uint64_t(int, std::uint64_t)>& depth_run) {
  const int max_depth = std::max(kMinDepth + 2, n);
  const int stretch = max_depth + 1;
  std::string out;
  std::format_to(std::back_inserter(out), "stretch tree of depth {}\t check: {}\n", stretch,
                 check_tree(*make_tree(stretch)));
  const auto long_lived = make_tree(max_depth);
  for (int depth = kMinDepth; depth <= max_depth; depth += 2) {
    const std::uint64_t iterations = std::uint64_t{1} << (max_depth - depth + kMinDepth);
    std::format_to(std::back_inserter(out), "{}\t trees of depth {}\t check: {}\n", iterations,
                   depth, depth_run(depth, iterations));
  }
  std::format_to(std::back_inserter(out), "long lived tree of depth {}\t check: {}\n", max_depth,
                 check_tree(*long_lived));
  return out;
}

void mandelbrot_row(std::size_t n, std::size_t y, std::span<std::uint8_t> row) {
  const double size = static_cast<double>(n);
  const double ci = 2.0 * static_cast<double>(y) / size - 1.0;
  for (std::size_t x = 0; x < n; ++x) {
    const double cr = 2.0 * static_cast<double>(x) / size - 1.5;
    double zr = 0.0;
    double zi = 0.0;
    double tr = 0.0;
    double ti = 0.0;
    for (int i = 0; i < 50 && tr + ti <= 4.0; ++i) {
      zi = 2.0 * zr * zi + ci;
      zr = tr - ti + cr;
      tr = zr * zr;
      ti = zi * zi;
    }
    if (tr + ti <= 4.0) {
      row[x >> 3] |= static_cast<std::uint8_t>(0x80 >> (x & 7));
    }
  }
}

std::span<const Kernel> bg_kernels() { return kBgKernels; }

}  // namespace kernels
