// Race: nbody (java)
// Extracted from suite/java/src/main/java/kernels/BenchmarksGame.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

import java.util.Locale;

final class BenchmarksGame {

  static final double PI = 3.141592653589793;

  static final double SOLAR_MASS = 4 * PI * PI;

  static final double DAYS_PER_YEAR = 365.24;

  static final class Body {
    double x, y, z, vx, vy, vz;
    final double mass;

    Body(double x, double y, double z, double vx, double vy, double vz, double mass) {
      this.x = x;
      this.y = y;
      this.z = z;
      this.vx = vx;
      this.vy = vy;
      this.vz = vz;
      this.mass = mass;
    }
  }

  static Body[] bodies() {
    return new Body[] {
      new Body(0, 0, 0, 0, 0, 0, SOLAR_MASS),
      new Body(
          4.84143144246472090e00,
          -1.16032004402742839e00,
          -1.03622044471123109e-01,
          1.66007664274403694e-03 * DAYS_PER_YEAR,
          7.69901118419740425e-03 * DAYS_PER_YEAR,
          -6.90460016972063023e-05 * DAYS_PER_YEAR,
          9.54791938424326609e-04 * SOLAR_MASS),
      new Body(
          8.34336671824457987e00,
          4.12479856412430479e00,
          -4.03523417114321381e-01,
          -2.76742510726862411e-03 * DAYS_PER_YEAR,
          4.99852801234917238e-03 * DAYS_PER_YEAR,
          2.30417297573763929e-05 * DAYS_PER_YEAR,
          2.85885980666130812e-04 * SOLAR_MASS),
      new Body(
          1.28943695621391310e01,
          -1.51111514016986312e01,
          -2.23307578892655734e-01,
          2.96460137564761618e-03 * DAYS_PER_YEAR,
          2.37847173959480950e-03 * DAYS_PER_YEAR,
          -2.96589568540237556e-05 * DAYS_PER_YEAR,
          4.36624404335156298e-05 * SOLAR_MASS),
      new Body(
          1.53796971148509165e01,
          -2.59193146099879641e01,
          1.79258772950371181e-01,
          2.68067772490389322e-03 * DAYS_PER_YEAR,
          1.62824170038242295e-03 * DAYS_PER_YEAR,
          -9.51592254519715870e-05 * DAYS_PER_YEAR,
          5.15138902046611451e-05 * SOLAR_MASS),
    };
  }

  static void offsetMomentum(Body[] bodies) {
    double px = 0;
    double py = 0;
    double pz = 0;
    for (var b : bodies) {
      px += b.vx * b.mass;
      py += b.vy * b.mass;
      pz += b.vz * b.mass;
    }
    var sun = bodies[0];
    sun.vx = -px / SOLAR_MASS;
    sun.vy = -py / SOLAR_MASS;
    sun.vz = -pz / SOLAR_MASS;
  }

  static double energy(Body[] bodies) {
    double e = 0;
    for (int i = 0; i < bodies.length; i++) {
      var bi = bodies[i];
      e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz);
      for (int j = i + 1; j < bodies.length; j++) {
        var bj = bodies[j];
        double dx = bi.x - bj.x;
        double dy = bi.y - bj.y;
        double dz = bi.z - bj.z;
        e -= (bi.mass * bj.mass) / Math.sqrt(dx * dx + dy * dy + dz * dz);
      }
    }
    return e;
  }

  static void advance(Body[] bodies, double dt, int steps) {
    for (int step = 0; step < steps; step++) {
      for (int i = 0; i < bodies.length; i++) {
        var bi = bodies[i];
        for (int j = i + 1; j < bodies.length; j++) {
          var bj = bodies[j];
          double dx = bi.x - bj.x;
          double dy = bi.y - bj.y;
          double dz = bi.z - bj.z;
          double d2 = dx * dx + dy * dy + dz * dz;
          double mag = dt / (d2 * Math.sqrt(d2));
          double bim = bi.mass * mag;
          double bjm = bj.mass * mag;
          bi.vx -= dx * bjm;
          bi.vy -= dy * bjm;
          bi.vz -= dz * bjm;
          bj.vx += dx * bim;
          bj.vy += dy * bim;
          bj.vz += dz * bim;
        }
      }
      for (var b : bodies) {
        b.x += dt * b.vx;
        b.y += dt * b.vy;
        b.z += dt * b.vz;
      }
    }
  }

  static String nbody(int steps) {
    var bodies = bodies();
    offsetMomentum(bodies);
    double before = energy(bodies);
    advance(bodies, 0.01, steps);
    return String.format(Locale.ROOT, "%.9f;%.9f", before, energy(bodies));
  }

}
