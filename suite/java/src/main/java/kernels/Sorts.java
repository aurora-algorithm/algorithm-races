package kernels;

import java.util.Arrays;

/** Sorting kernels, written the same way in every language (kernels/SPEC.md section 3). */
final class Sorts {
  private Sorts() {}

  static long[] builtin(long[] a) {
    Arrays.sort(a);
    return a;
  }

  static String[] builtin(String[] a) {
    Arrays.sort(a);
    return a;
  }

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
