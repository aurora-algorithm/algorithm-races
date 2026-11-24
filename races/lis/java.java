// Race: lis (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

import java.util.Arrays;

final class Algo {

  static String lis(long[] values) {
    var tails = new long[16];
    int length = 0;
    for (long x : values) {
      int i = Arrays.binarySearch(tails, 0, length, x);
      if (i < 0) {
        i = -(i + 1);
      }
      if (i == tails.length) {
        tails = Arrays.copyOf(tails, tails.length * 2);
      }
      tails[i] = x;
      if (i == length) {
        length++;
      }
    }
    return length + ":" + Gen.h31(Arrays.copyOf(tails, length));
  }

}
