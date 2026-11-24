package kernels;

import java.io.IOException;
import java.lang.management.GarbageCollectorMXBean;
import java.lang.management.ManagementFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.UnaryOperator;
import java.util.stream.Collectors;

/**
 * Kernel runner (kernels/SPEC.md section 1).
 *
 * <p>usage: {@code kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]}; prints one
 * JSON line with the checksum and the time of each repeat (kernel only).
 */
public final class Main {
  private Main() {}

  record Args(String kernel, int size, int repeats, int threads, Path data) {}

  record Context(int threads, ExecutorService pool) {}

  @FunctionalInterface
  interface Prepare<I> {
    I input(int size, Path data) throws IOException;
  }

  @FunctionalInterface
  interface Run<I, R> {
    R run(I input, Context ctx);
  }

  /** Input built before timing, a fresh copy per repeat, the timed call and its checksum. */
  record Kernel<I, R>(
      Prepare<I> prepare,
      UnaryOperator<I> fresh,
      Run<I, R> run,
      Function<R, String> checksum,
      Function<R, Map<String, Long>> extra) {
    Kernel(
        Prepare<I> prepare, UnaryOperator<I> fresh, Run<I, R> run, Function<R, String> checksum) {
      this(prepare, fresh, run, checksum, r -> Map.of());
    }

    static <R> Kernel<Integer, R> sized(Run<Integer, R> run, Function<R, String> checksum) {
      return new Kernel<>((n, data) -> n, n -> n, run, checksum);
    }

    static Kernel<long[], long[]> sort(UnaryOperator<long[]> sort) {
      return new Kernel<>(
          (n, data) -> Gen.ints(n), long[]::clone, (a, ctx) -> sort.apply(a), Main::sortedInts);
    }
  }

  static String sortedInts(long[] a) {
    return Long.toString(Gen.h31(a));
  }

  static String sortedStrings(String[] a) {
    var text = new StringBuilder();
    for (var s : a) {
      text.append(s).append('\n');
    }
    return Gen.fnv1a(text.toString());
  }

  static Map<String, Kernel<?, ?>> kernels() {
    return Map.ofEntries(
        Map.entry("sort-int", Kernel.sort(Sorts::builtin)),
        Map.entry(
            "sort-str",
            new Kernel<>(
                (n, data) -> Gen.strings(n),
                String[]::clone,
                (a, ctx) -> Sorts.builtin(a),
                Main::sortedStrings)),
        Map.entry("quicksort", Kernel.sort(Sorts::quicksort)),
        Map.entry("mergesort", Kernel.sort(Sorts::mergesort)),
        Map.entry("heapsort", Kernel.sort(Sorts::heapsort)),
        Map.entry("radixsort", Kernel.sort(Sorts::radixsort)),
        Map.entry(
            "binary-trees", Kernel.sized((n, ctx) -> BenchmarksGame.binaryTrees(n), Gen::fnv1a)),
        Map.entry("nbody", Kernel.sized((n, ctx) -> BenchmarksGame.nbody(n), s -> s)),
        Map.entry("mandelbrot", Kernel.sized((n, ctx) -> BenchmarksGame.mandelbrot(n), Gen::fnv1a)),
        Map.entry("fannkuch", Kernel.sized((n, ctx) -> BenchmarksGame.fannkuch(n), s -> s)),
        Map.entry(
            "spectral-norm", Kernel.sized((n, ctx) -> BenchmarksGame.spectralNorm(n), s -> s)),
        Map.entry(
            "regex-log",
            new Kernel<>(
                (n, data) -> Misc.logInput(Gen.logText(n)),
                in -> in,
                (in, ctx) -> Misc.regexLog(in),
                s -> s)),
        Map.entry(
            "hashmap",
            new Kernel<>(
                (n, data) -> Misc.hashmapInput(n),
                in -> in,
                (in, ctx) -> Misc.hashmap(in),
                s -> s)),
        Map.entry(
            "sha256",
            new Kernel<>(
                (n, data) -> Gen.shaBuffer(n), b -> b, (b, ctx) -> Misc.sha256(b), s -> s)),
        Map.entry("sieve", Kernel.sized((n, ctx) -> Misc.sieve(n), s -> s)),
        Map.entry(
            "matmul",
            new Kernel<>(
                (n, data) -> Gen.matrices(n),
                m -> m,
                (m, ctx) -> Misc.matmul(m),
                Misc::matmulChecksum)),
        Map.entry(
            "json",
            new Kernel<>(
                (n, data) ->
                    new Misc.JsonInput(
                        Misc.mapper(), Files.readString(data.resolve("orders-" + n + ".json"))),
                in -> in,
                (in, ctx) -> Misc.json(in),
                Misc::jsonChecksum)),
        Map.entry(
            "mergesort-mt",
            new Kernel<>(
                (n, data) -> Gen.ints(n),
                long[]::clone,
                (a, ctx) -> ParallelKernels.mergesort(a, ctx.threads(), ctx.pool()),
                Main::sortedInts)),
        Map.entry(
            "mandelbrot-mt",
            Kernel.sized(
                (n, ctx) -> ParallelKernels.mandelbrot(n, ctx.threads(), ctx.pool()), Gen::fnv1a)),
        Map.entry(
            "binary-trees-mt",
            Kernel.sized(
                (n, ctx) -> ParallelKernels.binaryTrees(n, ctx.threads(), ctx.pool()),
                Gen::fnv1a)));
  }

  record Measurement(String checksum, List<Double> timesMs, Map<String, Long> extra) {}

  static <I, R> Measurement measure(Kernel<I, R> kernel, Args args, Context ctx)
      throws IOException {
    var base = kernel.prepare().input(args.size(), args.data());
    var times = new ArrayList<Double>();
    R result = null;
    for (int i = 0; i < args.repeats(); i++) {
      var work = kernel.fresh().apply(base);
      long start = System.nanoTime();
      result = kernel.run().run(work, ctx);
      times.add((System.nanoTime() - start) / 1e6);
    }
    return new Measurement(kernel.checksum().apply(result), times, kernel.extra().apply(result));
  }

  static String runtimeInfo() {
    var gcs =
        ManagementFactory.getGarbageCollectorMXBeans().stream()
            .map(GarbageCollectorMXBean::getName)
            .collect(Collectors.joining(", "));
    var vendor = System.getProperty("java.vendor.version", "");
    return "%s %s%s, GC: %s"
        .formatted(
            System.getProperty("java.vm.name"),
            System.getProperty("java.version"),
            vendor.isEmpty() ? "" : " (" + vendor + ")",
            gcs.isEmpty() ? "unknown" : gcs);
  }

  static String jsonLine(Args args, Measurement m, String runtime) {
    var times =
        m.timesMs().stream()
            .map(t -> String.format(Locale.ROOT, "%.3f", t))
            .collect(Collectors.joining(","));
    var extra =
        m.extra().entrySet().stream()
            .map(e -> quote(e.getKey()) + ":" + e.getValue())
            .collect(Collectors.joining(","));
    return String.format(
        Locale.ROOT,
        "{\"kernel\":%s,\"size\":%d,\"threads\":%d,\"checksum\":%s,\"times_ms\":[%s],\"runtime\":%s%s}",
        quote(args.kernel()),
        args.size(),
        args.threads(),
        quote(m.checksum()),
        times,
        quote(runtime),
        extra.isEmpty() ? "" : ",\"extra\":{" + extra + "}");
  }

  static String quote(String s) {
    return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
  }

  static Args parse(String[] argv) {
    if (argv.length == 0) {
      throw new IllegalArgumentException("missing KERNEL");
    }
    int size = -1;
    int repeats = 1;
    int threads = 1;
    var data = Path.of(".");
    for (int i = 1; i < argv.length; i += 2) {
      if (i + 1 >= argv.length) {
        throw new IllegalArgumentException("missing value for " + argv[i]);
      }
      var value = argv[i + 1];
      switch (argv[i]) {
        case "--size" -> size = Integer.parseInt(value);
        case "--repeats" -> repeats = Integer.parseInt(value);
        case "--threads" -> threads = Integer.parseInt(value);
        case "--data" -> data = Path.of(value);
        default -> throw new IllegalArgumentException("unknown option " + argv[i]);
      }
    }
    if (size < 0 || repeats < 1 || threads < 1) {
      throw new IllegalArgumentException("need --size N >= 0, --repeats >= 1, --threads >= 1");
    }
    return new Args(argv[0], size, repeats, threads, data);
  }

  /**
   * The work kernels (Work.java), looked up by a name read at run time: native-image folds a
   * constant Class.forName and would compile AWT and the JNI libraries into every binary. In a
   * native image the lookup finds nothing and they are unknown kernels (java-native skips the work
   * group).
   */
  private static final String WORK_CLASS = "kernels.Work";

  static Object callWork(String method) throws ReflectiveOperationException {
    var name = System.getProperty("kernels.work.class", WORK_CLASS);
    return Class.forName(name).getDeclaredMethod(method).invoke(null);
  }

  @SuppressWarnings("unchecked")
  static Map<String, Kernel<?, ?>> workKernels() {
    try {
      return (Map<String, Kernel<?, ?>>) callWork("kernels");
    } catch (ReflectiveOperationException e) {
      return Map.of();
    }
  }

  static String workLibraries() {
    try {
      return (String) callWork("libraries");
    } catch (ReflectiveOperationException e) {
      return "";
    }
  }

  static ExecutorService startPool(int threads) {
    var pool =
        new ThreadPoolExecutor(threads, threads, 0, TimeUnit.SECONDS, new LinkedBlockingQueue<>());
    pool.prestartAllCoreThreads();
    return pool;
  }

  public static void main(String[] argv) throws IOException {
    Args args;
    try {
      args = parse(argv);
    } catch (IllegalArgumentException e) {
      System.err.println(e.getMessage());
      System.err.println("usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]");
      System.exit(2);
      return;
    }
    var kernel = kernels().get(args.kernel());
    var runtime = runtimeInfo();
    if (kernel == null) {
      kernel = Algo.kernels().get(args.kernel());
    }
    if (kernel == null) {
      kernel = Puzzle.kernels().get(args.kernel());
    }
    if (kernel == null) {
      kernel = Ways.kernels().get(args.kernel());
    }
    if (kernel == null) {
      kernel = Arena.kernels().get(args.kernel());
    }
    if (kernel == null) {
      kernel = Autocomplete.kernels().get(args.kernel());
    }
    if (kernel == null) {
      kernel = workKernels().get(args.kernel());
      runtime += ", " + workLibraries();
    }
    if (kernel == null) {
      System.err.println("unknown kernel " + args.kernel());
      System.exit(2);
    }
    Measurement m;
    try (var pool = startPool(args.threads())) {
      m = measure(kernel, args, new Context(args.threads(), pool));
    }
    System.out.println(jsonLine(args, m, runtime));
  }
}
