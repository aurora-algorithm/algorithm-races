// Race: binary-search (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  record Search(long[] a, long[] queries) {}

  static Search sortedArray(int n) {
    var rng = new Gen.XorShift32();
    var a = new long[n];
    long v = 0;
    for (int i = 0; i < n; i++) {
      v += 1 + rng.next() % 4;
      a[i] = v;
    }
    var queries = new long[n];
    for (int i = 0; i < n; i++) {
      queries[i] = rng.next() % (v + 1);
    }
    return new Search(a, queries);
  }

  static String binarySearch(Search s) {
    var a = s.a();
    int n = a.length;
    long hits = 0;
    long total = 0;
    for (long q : s.queries()) {
      int lo = 0;
      int hi = n;
      while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (a[mid] < q) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      if (lo < n && a[lo] == q) {
        hits++;
      }
      total += lo;
    }
    return hits + ":" + total;
  }

}
