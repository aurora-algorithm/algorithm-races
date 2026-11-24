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

/**
 * The builtins group (kernels/builtins-NOTES.md): the JDK's ways of doing one job, one way per
 * kernel, named JOB.java.WAY. Every way of a job gets the same input and prints the same checksum.
 */
final class Ways {
  private Ways() {}

  private static final String LANG = "java";

  private static final Map<String, IntFunction<long[]>> INT_INPUTS =
      Map.of(
          "bsort-int", Gen::ints,
          "bsort-int-sorted", Ways::intsSorted,
          "bsort-int-reversed", Ways::intsReversed);

  private static final Map<String, IntFunction<String[]>> STR_INPUTS =
      Map.of(
          "bsort-str", Gen::strings,
          "bsort-str-sorted", Ways::stringsSorted,
          "bsort-str-reversed", Ways::stringsReversed);

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

  static List<Long> boxed(long[] a) {
    var list = new ArrayList<Long>(a.length);
    for (long v : a) {
      list.add(v);
    }
    return list;
  }

  static long[] unboxed(List<Long> list) {
    var a = new long[list.size()];
    for (int i = 0; i < a.length; i++) {
      a[i] = list.get(i);
    }
    return a;
  }

  static long[] arraysSort(long[] a) {
    Arrays.sort(a);
    return a;
  }

  static long[] arraysParallelSort(long[] a) {
    Arrays.parallelSort(a);
    return a;
  }

  static <T extends Comparable<? super T>> List<T> collectionsSort(List<T> list) {
    Collections.sort(list);
    return list;
  }

  static <T extends Comparable<? super T>> List<T> streamSorted(List<T> list) {
    return list.stream().sorted().toList();
  }

  static long[] longStreamSorted(long[] a) {
    return Arrays.stream(a).sorted().toArray();
  }

  static String[] arraysSort(String[] a) {
    Arrays.sort(a);
    return a;
  }

  static String[] arraysParallelSort(String[] a) {
    Arrays.parallelSort(a);
    return a;
  }

  /** Arrays.parallelSort runs on the common pool, sized by the JVM from the CPUs it may use. */
  static Map<String, Long> poolThreads(Object result) {
    return Map.of("pool_threads", (long) ForkJoinPool.getCommonPoolParallelism());
  }

  static String sortedStrings(List<String> list) {
    return Main.sortedStrings(list.toArray(String[]::new));
  }

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

  static String stringJoin(String[] parts) {
    return String.join(",", parts);
  }

  static String stringBuilder(String[] parts) {
    var sb = new StringBuilder();
    for (int i = 0; i < parts.length; i++) {
      if (i > 0) {
        sb.append(',');
      }
      sb.append(parts[i]);
    }
    return sb.toString();
  }

  static String stringBuilderCapacity(String[] parts) {
    int total = Math.max(parts.length - 1, 0);
    for (var p : parts) {
      total += p.length();
    }
    var sb = new StringBuilder(total);
    for (int i = 0; i < parts.length; i++) {
      if (i > 0) {
        sb.append(',');
      }
      sb.append(parts[i]);
    }
    return sb.toString();
  }

  static String stringJoiner(String[] parts) {
    var joiner = new StringJoiner(",");
    for (var p : parts) {
      joiner.add(p);
    }
    return joiner.toString();
  }

  static String collectorsJoining(String[] parts) {
    return Arrays.stream(parts).collect(Collectors.joining(","));
  }

  private static <R> Main.Kernel<long[], R> ints(
      IntFunction<long[]> make, Main.Run<long[], R> run, Function<R, String> c) {
    return new Main.Kernel<>((n, data) -> make.apply(n), long[]::clone, run, c);
  }

  private static Main.Kernel<List<Long>, List<Long>> boxedInts(
      IntFunction<long[]> make, UnaryOperator<List<Long>> sort) {
    return new Main.Kernel<>(
        (n, data) -> boxed(make.apply(n)),
        ArrayList::new,
        (list, ctx) -> sort.apply(list),
        list -> Main.sortedInts(unboxed(list)));
  }

  static Map<String, Main.Kernel<?, ?>> kernels() {
    var table = new HashMap<String, Main.Kernel<?, ?>>();
    INT_INPUTS.forEach((job, make) -> {
      String p = job + "." + LANG + ".";
      table.put(p + "Arrays.sort", ints(make, (a, ctx) -> arraysSort(a), Main::sortedInts));
      table.put(
          p + "Arrays.parallelSort",
          new Main.Kernel<>(
              (n, data) -> make.apply(n),
              long[]::clone,
              (a, ctx) -> arraysParallelSort(a),
              Main::sortedInts,
              Ways::poolThreads));
      table.put(p + "Collections.sort", boxedInts(make, Ways::collectionsSort));
      table.put(p + "Stream.sorted", boxedInts(make, Ways::streamSorted));
      table.put(
          p + "LongStream.sorted", ints(make, (a, ctx) -> longStreamSorted(a), Main::sortedInts));
    });
    STR_INPUTS.forEach((job, make) -> {
      String p = job + "." + LANG + ".";
      table.put(
          p + "Arrays.sort",
          new Main.Kernel<>(
              (n, data) -> make.apply(n),
              String[]::clone,
              (a, ctx) -> arraysSort(a),
              Main::sortedStrings));
      table.put(
          p + "Arrays.parallelSort",
          new Main.Kernel<>(
              (n, data) -> make.apply(n),
              String[]::clone,
              (a, ctx) -> arraysParallelSort(a),
              Main::sortedStrings,
              Ways::poolThreads));
      table.put(p + "Collections.sort", strings(make, Ways::collectionsSort));
      table.put(p + "Stream.sorted", strings(make, Ways::streamSorted));
    });
    Map<String, Function<String[], String>> joins =
        Map.of(
            "String.join", Ways::stringJoin,
            "StringBuilder", Ways::stringBuilder,
            "StringBuilder.capacity", Ways::stringBuilderCapacity,
            "StringJoiner", Ways::stringJoiner,
            "Collectors.joining", Ways::collectorsJoining);
    joins.forEach((way, join) ->
        table.put(
            "bjoin." + LANG + "." + way,
            new Main.Kernel<>(
                (n, data) -> joinParts(n),
                parts -> parts,
                (parts, ctx) -> join.apply(parts),
                Ways::joinedChecksum)));
    table.putAll(WaysMaps.kernels());
    return table;
  }

  private static Main.Kernel<List<String>, List<String>> strings(
      IntFunction<String[]> make, UnaryOperator<List<String>> sort) {
    return new Main.Kernel<>(
        (n, data) -> new ArrayList<>(Arrays.asList(make.apply(n))),
        ArrayList::new,
        (list, ctx) -> sort.apply(list),
        Ways::sortedStrings);
  }
}
