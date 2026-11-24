// Race: mandelbrot (java)
// Extracted from suite/java/src/main/java/kernels/BenchmarksGame.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

final class BenchmarksGame {

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

}
