// Race: bsort-int-reversed.goose.sort (java)
// Extracted from suite/java/src/main/java/kernels/Ways.java — build and run via suite/run.py

package kernels;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringJoiner;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Function;
import java.util.function.IntFunction;
import java.util.function.UnaryOperator;
import java.util.stream.Collectors;

import java.util.Arrays;

final class Ways {

  static long[] intsSorted(int n) {
    var a = Gen.ints(n);
    Arrays.sort(a);
    return a;
  }

  static long[] intsReversed(int n) {
    var a = intsSorted(n);
    for (int i = 0, j = a.length - 1; i < j; i++, j--) {
      long t = a[i];
      a[i] = a[j];
      a[j] = t;
    }
    return a;
  }

}
