// Race: collatz (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

final class Puzzle {

  static String collatz(int n) {
    var cache = new int[n];
    cache[1] = 1;
    long best = 1;
    long bestLen = 1;
    long total = 1;
    for (int start = 2; start < n; start++) {
      long x = start;
      int steps = 0;
      while (x >= start) {
        x = x % 2 == 0 ? x / 2 : 3 * x + 1;
        steps++;
      }
      int length = steps + cache[(int) x];
      cache[start] = length;
      total += length;
      if (length > bestLen) {
        best = start;
        bestLen = length;
      }
    }
    return n + ":" + best + ":" + bestLen + ":" + total;
  }

}
