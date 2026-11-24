// Race: nbody (cpp)
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

Outcome nbody_kernel(const Args& args) {
  return outcome(measure(args.repeats, [&] { return nbody(args.size); }));
}

}  // namespace kernels
