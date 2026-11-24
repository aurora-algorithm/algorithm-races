// Race: radixsort (java)
// Extracted from suite/java/src/main/java/kernels/Sorts.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;

final class Sorts {

  /** LSD, 8 bits per pass; after four passes the result is back in {@code a}. */
  static long[] radixsort(long[] a) {
    var src = a;
    var dst = new long[a.length];
    for (int shift = 0; shift < 32; shift += 8) {
      var count = new int[256];
      for (long v : src) {
        count[(int) (v >> shift) & 255]++;
      }
      var pos = new int[256];
      int total = 0;
      for (int d = 0; d < 256; d++) {
        pos[d] = total;
        total += count[d];
      }
      for (long v : src) {
        dst[pos[(int) (v >> shift) & 255]++] = v;
      }
      var swap = src;
      src = dst;
      dst = swap;
    }
    return src;
  }

}
