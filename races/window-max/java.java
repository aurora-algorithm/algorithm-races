// Race: window-max (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

final class Puzzle {

  private static final long MASK = 0xFFFFFFFFL;

  private static final int[] WINDOWS = {10, 100, 1000, 10000, 100000};

  static String windowMax(long[] values) {
    int n = values.length;
    var dq = new int[n];
    var result = new StringBuilder().append(n);
    for (int w : WINDOWS) {
      int head = 0;
      int tail = 0;
      long h = 0;
      for (int i = 0; i < n; i++) {
        long v = values[i];
        while (tail > head && values[dq[tail - 1]] <= v) {
          tail--;
        }
        dq[tail++] = i;
        if (dq[head] <= i - w) {
          head++;
        }
        if (i >= w - 1) {
          h = (h * 31 + values[dq[head]]) & MASK;
        }
      }
      result.append(':').append(h);
    }
    return result.toString();
  }

}
