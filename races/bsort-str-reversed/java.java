// Race: bsort-str-reversed.rust.sort (java)
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

import java.util.Collections;

final class Ways {

  static String[] stringsSorted(int n) {
    var a = Gen.strings(n);
    Arrays.sort(a);
    return a;
  }

  static String[] stringsReversed(int n) {
    var a = stringsSorted(n);
    Collections.reverse(Arrays.asList(a));
    return a;
  }

}
