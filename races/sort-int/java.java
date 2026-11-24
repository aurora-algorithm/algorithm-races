// Race: sort-int (java)
// Extracted from suite/java/src/main/java/kernels/Sorts.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;

import java.util.Arrays;

final class Sorts {

  static long[] builtin(long[] a) {
    Arrays.sort(a);
    return a;
  }

  static String[] builtin(String[] a) {
    Arrays.sort(a);
    return a;
  }

}
