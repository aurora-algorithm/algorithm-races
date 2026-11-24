// Race: quicksort (java)
// Extracted from suite/java/src/main/java/kernels/Sorts.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;

final class Sorts {

  static long[] quicksort(long[] a) {
    quicksort(a, 0, a.length - 1);
    return a;
  }

  /** Hoare partition, middle pivot; recurses into the smaller side so the stack stays shallow. */
  private static void quicksort(long[] a, int lo, int hi) {
    while (lo < hi) {
      long pivot = a[lo + (hi - lo) / 2];
      int i = lo - 1;
      int j = hi + 1;
      while (true) {
        do {
          i++;
        } while (a[i] < pivot);
        do {
          j--;
        } while (a[j] > pivot);
        if (i >= j) {
          break;
        }
        long t = a[i];
        a[i] = a[j];
        a[j] = t;
      }
      if (j - lo < hi - j) {
        quicksort(a, lo, j);
        lo = j + 1;
      } else {
        quicksort(a, j + 1, hi);
        hi = j;
      }
    }
  }

}
