// Race: sort-str (java)
// Extracted from suite/java/src/main/java/kernels/Main.java — build and run via suite/run.py

package kernels;

import java.util.Arrays;

final class SortStr {
  static String[] sortStr(String[] a) {
    Arrays.sort(a);
    return a;
  }
}
