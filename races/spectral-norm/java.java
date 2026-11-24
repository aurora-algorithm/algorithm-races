// Race: spectral-norm (java)
// Extracted from suite/java/src/main/java/kernels/BenchmarksGame.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

import java.util.Arrays;

import java.util.Locale;

final class BenchmarksGame {

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
