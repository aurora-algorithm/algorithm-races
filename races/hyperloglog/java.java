// Race: hyperloglog (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

final class Puzzle {

  /**
   * MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
   */
  static int fmix32(int h) {
    h ^= h >>> 16;
    h *= 0x85ebca6b;
    h ^= h >>> 13;
    h *= 0xc2b2ae35;
    h ^= h >>> 16;
    return h;
  }

  private static final int HLL_P = 14;

  private static final int HLL_M = 1 << HLL_P;

  static String hyperloglog(int n) {
    var rng = new Gen.XorShift32();
    int universe = Math.max(n / 4, 1);
    var registers = new byte[HLL_M];
    for (int k = 0; k < n; k++) {
      int h = fmix32(Integer.remainderUnsigned((int) rng.next(), universe));
      int w = h << HLL_P;
      int rank = w != 0 ? Integer.numberOfLeadingZeros(w) + 1 : 32 - HLL_P + 1;
      int idx = h >>> (32 - HLL_P);
      if (rank > registers[idx]) {
        registers[idx] = (byte) rank;
      }
    }
    double z = 0.0;
    int zeros = 0;
    var values = new long[HLL_M];
    for (int i = 0; i < HLL_M; i++) {
      int r = registers[i];
      z += Math.pow(2.0, -r);
      if (r == 0) {
        zeros++;
      }
      values[i] = r;
    }
    double m = HLL_M;
    double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if (estimate <= 2.5 * m && zeros > 0) {
      estimate = m * Math.log(m / zeros);
    }
    return n + ":" + (long) Math.floor(estimate + 0.5) + ":" + Gen.h31(values);
  }

}
