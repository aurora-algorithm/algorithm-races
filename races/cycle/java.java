// Race: cycle (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

final class Puzzle {

  private static final long CYCLE_MOD = 67108859L;

  private static final long CYCLE_C = 12345L;

  static long[] cycleStarts(int n) {
    var rng = new Gen.XorShift32();
    var starts = new long[n];
    for (int i = 0; i < n; i++) {
      starts[i] = rng.next() % CYCLE_MOD;
    }
    return starts;
  }

  static long step(long x) {
    return (x * x + CYCLE_C) % CYCLE_MOD;
  }

  static String cycle(long[] starts) {
    long totalMu = 0;
    long totalLam = 0;
    long longest = 0;
    for (long x0 : starts) {
      long tortoise = step(x0);
      long hare = step(tortoise);
      while (tortoise != hare) {
        tortoise = step(tortoise);
        hare = step(step(hare));
      }
      long mu = 0;
      tortoise = x0;
      while (tortoise != hare) {
        tortoise = step(tortoise);
        hare = step(hare);
        mu++;
      }
      long lam = 1;
      hare = step(tortoise);
      while (tortoise != hare) {
        hare = step(hare);
        lam++;
      }
      totalMu += mu;
      totalLam += lam;
      longest = Math.max(longest, mu + lam);
    }
    return starts.length + ":" + totalMu + ":" + totalLam + ":" + longest;
  }

}
