// Race: heapsort (java)
// Extracted from suite/java/src/main/java/kernels/Sorts.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;

final class Sorts {

  static long[] heapsort(long[] a) {
    int n = a.length;
    for (int start = n / 2 - 1; start >= 0; start--) {
      siftDown(a, start, n);
    }
    for (int end = n - 1; end > 0; end--) {
      long t = a[0];
      a[0] = a[end];
      a[end] = t;
      siftDown(a, 0, end);
    }
    return a;
  }

  private static void siftDown(long[] a, int root, int end) {
    while (2 * root + 1 < end) {
      int child = 2 * root + 1;
      if (child + 1 < end && a[child] < a[child + 1]) {
        child++;
      }
      if (a[root] < a[child]) {
        long t = a[root];
        a[root] = a[child];
        a[child] = t;
        root = child;
      } else {
        return;
      }
    }
  }

}
