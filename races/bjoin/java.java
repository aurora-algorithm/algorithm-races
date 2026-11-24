// Race: bjoin.go.strings.Join (java)
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

final class Ways {

  static String joinedChecksum(String s) {
    return s.length() + ":" + Gen.fnv1a(s);
  }

  static String[] joinParts(int n) {
    var values = Gen.ints(n);
    var parts = new String[n];
    for (int i = 0; i < n; i++) {
      parts[i] = Long.toString(values[i]);
    }
    return parts;
  }

}
