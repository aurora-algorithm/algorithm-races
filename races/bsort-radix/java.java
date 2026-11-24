// Race: bsort-int.go.zermelo.Sort (java)
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

import java.util.function.Function;

import java.util.function.IntFunction;

final class Ways {

  private static <R> Main.Kernel<long[], R> ints(
      IntFunction<long[]> make, Main.Run<long[], R> run, Function<R, String> c) {
    return new Main.Kernel<>((n, data) -> make.apply(n), long[]::clone, run, c);
  }

}
