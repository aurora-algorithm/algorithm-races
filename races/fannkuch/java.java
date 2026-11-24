// Race: fannkuch (java)
// Extracted from suite/java/src/main/java/kernels/BenchmarksGame.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

final class BenchmarksGame {

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

}
