package kernels;

import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

/**
 * Benchmarks Game kernels, single-threaded (kernels/SPEC.md section 4). Floating-point operations
 * keep the order written there, so every language prints the same digits.
 */
final class BenchmarksGame {
  static final int MIN_DEPTH = 4;

  private BenchmarksGame() {}

  record Node(Node left, Node right) {}

  static Node makeTree(int depth) {
    if (depth == 0) {
      return new Node(null, null);
    }
    return new Node(makeTree(depth - 1), makeTree(depth - 1));
  }

  static int checkTree(Node node) {
    if (node.left() == null) {
      return 1;
    }
    return 1 + checkTree(node.left()) + checkTree(node.right());
  }

  static int depthChecks(int depth, int iterations) {
    int check = 0;
    for (int i = 0; i < iterations; i++) {
      check += checkTree(makeTree(depth));
    }
    return check;
  }

  /** The output text; {@code depthRun} sums the checks of (depth, iterations) trees. */
  static String binaryTreesLines(int n, IntBinaryOperator depthRun) {
    int maxDepth = Math.max(MIN_DEPTH + 2, n);
    int stretch = maxDepth + 1;
    var out = new StringBuilder();
    out.append(
        "stretch tree of depth " + stretch + "\t check: " + checkTree(makeTree(stretch)) + "\n");
    var longLived = makeTree(maxDepth);
    for (int depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
      int iterations = 1 << (maxDepth - depth + MIN_DEPTH);
      int check = depthRun.applyAsInt(depth, iterations);
      out.append(iterations + "\t trees of depth " + depth + "\t check: " + check + "\n");
    }
    out.append("long lived tree of depth " + maxDepth + "\t check: " + checkTree(longLived) + "\n");
    return out.toString();
  }

  static String binaryTrees(int n) {
    return binaryTreesLines(n, BenchmarksGame::depthChecks);
  }

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

  static int rowBytes(int n) {
    return (n + 7) / 8;
  }

  /** Writes row {@code y} of the N x N bitmap (MSB first) into {@code out}. */
  static void mandelbrotRow(int n, int y, byte[] out) {
    int offset = y * rowBytes(n);
    double ci = 2.0 * y / n - 1.0;
    for (int x = 0; x < n; x++) {
      double cr = 2.0 * x / n - 1.5;
      double zr = 0;
      double zi = 0;
      double tr = 0;
      double ti = 0;
      int i = 0;
      while (i < 50 && tr + ti <= 4.0) {
        zi = 2.0 * zr * zi + ci;
        zr = tr - ti + cr;
        tr = zr * zr;
        ti = zi * zi;
        i++;
      }
      if (tr + ti <= 4.0) {
        out[offset + (x >> 3)] |= (byte) (0x80 >> (x & 7));
      }
    }
  }

  static byte[] mandelbrot(int n) {
    var out = new byte[n * rowBytes(n)];
    for (int y = 0; y < n; y++) {
      mandelbrotRow(n, y, out);
    }
    return out;
  }

  static String fannkuch(int n) {
    var perm1 = new int[n];
    for (int i = 0; i < n; i++) {
      perm1[i] = i;
    }
    var perm = new int[n];
    var count = new int[n];
    int maxFlips = 0;
    int checksum = 0;
    int permCount = 0;
    int r = n;
    while (true) {
      while (r != 1) {
        count[r - 1] = r;
        r--;
      }
      System.arraycopy(perm1, 0, perm, 0, n);
      int flips = 0;
      int k = perm[0];
      while (k != 0) {
        for (int lo = 0, hi = k; lo < hi; lo++, hi--) {
          int t = perm[lo];
          perm[lo] = perm[hi];
          perm[hi] = t;
        }
        flips++;
        k = perm[0];
      }
      maxFlips = Math.max(maxFlips, flips);
      checksum += permCount % 2 == 0 ? flips : -flips;
      while (true) {
        if (r == n) {
          return checksum + ":" + maxFlips;
        }
        int perm0 = perm1[0];
        System.arraycopy(perm1, 1, perm1, 0, r);
        perm1[r] = perm0;
        count[r]--;
        if (count[r] > 0) {
          break;
        }
        r++;
      }
      permCount++;
    }
  }

  static double evalA(long i, long j) {
    return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);
  }

  static double[] mulAv(double[] u) {
    int n = u.length;
    var out = new double[n];
    for (int i = 0; i < n; i++) {
      double s = 0;
      for (int j = 0; j < n; j++) {
        s += evalA(i, j) * u[j];
      }
      out[i] = s;
    }
    return out;
  }

  static double[] mulAtv(double[] u) {
    int n = u.length;
    var out = new double[n];
    for (int i = 0; i < n; i++) {
      double s = 0;
      for (int j = 0; j < n; j++) {
        s += evalA(j, i) * u[j];
      }
      out[i] = s;
    }
    return out;
  }

  static double[] mulAtAv(double[] u) {
    return mulAtv(mulAv(u));
  }

  static String spectralNorm(int n) {
    var u = new double[n];
    Arrays.fill(u, 1.0);
    var v = u;
    for (int i = 0; i < 10; i++) {
      v = mulAtAv(u);
      u = mulAtAv(v);
    }
    double vBv = 0;
    double vv = 0;
    for (int i = 0; i < n; i++) {
      vBv += u[i] * v[i];
      vv += v[i] * v[i];
    }
    return String.format(Locale.ROOT, "%.9f", Math.sqrt(vBv / vv));
  }
}
