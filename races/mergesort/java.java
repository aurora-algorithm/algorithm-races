// Race: mergesort (java)
// Extracted from suite/java/src/main/java/kernels/Sorts.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;

final class Sorts {

  static long[] mergesort(long[] a) {
    mergesort(a, new long[a.length], 0, a.length);
    return a;
  }

  static void mergesort(long[] a, long[] aux, int lo, int hi) {
    if (hi - lo < 2) {
      return;
    }
    int mid = lo + (hi - lo) / 2;
    mergesort(a, aux, lo, mid);
    mergesort(a, aux, mid, hi);
    merge(a, aux, lo, mid, hi);
  }

  static void merge(long[] a, long[] aux, int lo, int mid, int hi) {
    System.arraycopy(a, lo, aux, lo, hi - lo);
    int i = lo;
    int j = mid;
    int k = lo;
    while (i < mid && j < hi) {
      if (aux[j] < aux[i]) {
        a[k++] = aux[j++];
      } else {
        a[k++] = aux[i++];
      }
    }
    while (i < mid) {
      a[k++] = aux[i++];
    }
    while (j < hi) {
      a[k++] = aux[j++];
    }
  }

}
