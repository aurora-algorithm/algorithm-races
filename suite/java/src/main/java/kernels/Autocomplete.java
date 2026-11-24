package kernels;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

/**
 * The autocomplete kernels (kernels/SPEC.md section 15): the first 10 words that start with a typed
 * prefix, from a sorted array with binary search, a 26-slot trie, or the same trie in one int
 * array.
 */
final class Autocomplete {
  private static final String WORDS_FILE = "words-web2.txt";
  private static final int TOP_K = 10;
  private static final int LETTERS = 26;
  private static final long MASK = 0xFFFFFFFFL;

  private Autocomplete() {}

  static Map<String, Main.Kernel<?, ?>> kernels() {
    return Map.ofEntries(
        Map.entry(
            "ac-sorted-build",
            new Main.Kernel<Build<String[]>, Build<String[]>>(
                Autocomplete::buildInput,
                b -> b.hold(b.words().clone()),
                (b, ctx) -> b.with(a -> Arrays.sort(a)),
                b -> checksum(Arrays.asList(b.built().value)))),
        Map.entry(
            "ac-trie-build",
            new Main.Kernel<Build<Trie>, Build<Trie>>(
                Autocomplete::buildInput,
                b -> b.hold(null),
                (b, ctx) -> b.hold(Trie.build(b.words())),
                b -> checksum(b.built().value.all(b.words())),
                b -> Map.of("nodes", (long) b.built().value.nodes()))),
        Map.entry(
            "ac-trie-arena-build",
            new Main.Kernel<Build<ArenaTrie>, Build<ArenaTrie>>(
                Autocomplete::buildInput,
                b -> b.hold(null),
                (b, ctx) -> b.hold(ArenaTrie.build(b.words())),
                b -> checksum(b.built().value.all(b.words())),
                b -> Map.of("nodes", (long) b.built().value.nodes()))),
        Map.entry("ac-sorted", queries(Autocomplete::sortedCompleter)),
        Map.entry("ac-trie", queries(w -> Trie.build(w).completer(w))),
        Map.entry("ac-trie-arena", queries(w -> ArenaTrie.build(w).completer(w))));
  }

  @FunctionalInterface
  interface Completer {
    void complete(String prefix, List<String> out);
  }

  @FunctionalInterface
  interface MakeCompleter {
    Completer make(String[] words);
  }

  record Queries(Completer completer, String[] prefixes) {}

  record Answers(int queries, long results, long h) {
    String checksum() {
      return queries + ":" + results + ":" + h;
    }
  }

  private static Main.Kernel<Queries, Answers> queries(MakeCompleter make) {
    return new Main.Kernel<>(
        (n, data) -> {
          var in = queryInput(n, data);
          return new Queries(make.make(in.words()), in.prefixes());
        },
        q -> q,
        (q, ctx) -> runQueries(q),
        Answers::checksum);
  }

  /**
   * The structure the last repeat built. The checksum reads it after timing, and the next repeat's
   * fresh step replaces it before its timer starts, so no repeat builds beside a live old one.
   */
  static final class Slot<T> {
    T value;
  }

  record Build<T>(String[] words, Slot<T> built) {
    Build<T> hold(T value) {
      built.value = value;
      return this;
    }

    Build<T> with(Consumer<T> work) {
      work.accept(built.value);
      return this;
    }
  }

  record Shuffled(String[] words, Gen.XorShift32 rng) {}

  static Shuffled shuffledWords(Path data) throws IOException {
    var words =
        Files.readAllLines(data.resolve(WORDS_FILE), StandardCharsets.US_ASCII).stream()
            .map(line -> line.endsWith("\r") ? line.substring(0, line.length() - 1) : line)
            .filter(line -> !line.isEmpty())
            .toArray(String[]::new);
    var rng = new Gen.XorShift32();
    for (int i = words.length - 1; i > 0; i--) {
      int j = (int) (rng.next() % (i + 1));
      var w = words[i];
      words[i] = words[j];
      words[j] = w;
    }
    return new Shuffled(words, rng);
  }

  static <T> Build<T> buildInput(int n, Path data) throws IOException {
    return new Build<>(Arrays.copyOf(shuffledWords(data).words(), n), new Slot<>());
  }

  record QueryInput(String[] words, String[] prefixes) {}

  static QueryInput queryInput(int n, Path data) throws IOException {
    var shuffled = shuffledWords(data);
    var words = shuffled.words();
    var rng = shuffled.rng();
    var prefixes = new String[n];
    for (int i = 0; i < n; i++) {
      var w = words[(int) (rng.next() % words.length)];
      int p = 2 + (int) (rng.next() % 4);
      prefixes[i] = w.substring(0, Math.min(p, w.length()));
    }
    return new QueryInput(words, prefixes);
  }

  private static long h31(long h, long v) {
    return (h * 31 + v) & MASK;
  }

  static long fold(long h, String w) {
    return h31(h31(h, w.length()), w.charAt(w.length() - 1));
  }

  static String checksum(List<String> inOrder) {
    long h = 0;
    for (var w : inOrder) {
      h = fold(h, w);
    }
    return inOrder.size() + ":" + h;
  }

  static Answers runQueries(Queries q) {
    long results = 0;
    long h = 0;
    var out = new ArrayList<String>(TOP_K);
    for (var prefix : q.prefixes()) {
      out.clear();
      q.completer().complete(prefix, out);
      results += out.size();
      for (var w : out) {
        h = fold(h, w);
      }
    }
    return new Answers(q.prefixes().length, results, h);
  }

  static Completer sortedCompleter(String[] shuffled) {
    var sorted = shuffled.clone();
    Arrays.sort(sorted);
    return (prefix, out) -> {
      int found = Arrays.binarySearch(sorted, prefix);
      int i = found >= 0 ? found : -found - 1;
      int end = Math.min(i + TOP_K, sorted.length);
      for (; i < end && sorted[i].startsWith(prefix); i++) {
        out.add(sorted[i]);
      }
    };
  }

  /** A trie of nodes with 26 child links and the index of the word that ends there (-1 none). */
  record Trie(Node root, int nodes) {
    static final class Node {
      final Node[] children = new Node[LETTERS];
      int word = -1;
    }

    static Trie build(String[] words) {
      var root = new Node();
      int nodes = 1;
      for (int idx = 0; idx < words.length; idx++) {
        var node = root;
        var w = words[idx];
        for (int i = 0; i < w.length(); i++) {
          int c = w.charAt(i) - 'a';
          var child = node.children[c];
          if (child == null) {
            child = new Node();
            node.children[c] = child;
            nodes++;
          }
          node = child;
        }
        node.word = idx;
      }
      return new Trie(root, nodes);
    }

    static void collect(Node node, String[] words, List<String> out, int limit) {
      if (node.word >= 0) {
        out.add(words[node.word]);
      }
      for (var child : node.children) {
        if (out.size() >= limit) {
          return;
        }
        if (child != null) {
          collect(child, words, out, limit);
        }
      }
    }

    List<String> all(String[] words) {
      var out = new ArrayList<String>(words.length);
      collect(root, words, out, words.length + 1);
      return out;
    }

    Completer completer(String[] words) {
      return (prefix, out) -> {
        var node = root;
        for (int i = 0; i < prefix.length(); i++) {
          node = node.children[prefix.charAt(i) - 'a'];
          if (node == null) {
            return;
          }
        }
        collect(node, words, out, TOP_K);
      };
    }
  }

  /**
   * The same trie in one int array, 27 ints per node: the word index, then 26 child links. Node 0
   * is the root and never anyone's child, so a child link of 0 means no child.
   */
  static final class ArenaTrie {
    private static final int STRIDE = 1 + LETTERS;
    private static final int FIRST_CAPACITY = 16;

    private int[] cells = new int[FIRST_CAPACITY * STRIDE];
    private int nodes;

    private ArenaTrie() {
      push();
    }

    int nodes() {
      return nodes;
    }

    private int push() {
      if ((nodes + 1) * STRIDE > cells.length) {
        cells = Arrays.copyOf(cells, 2 * cells.length);
      }
      cells[nodes * STRIDE] = -1;
      return nodes++;
    }

    static ArenaTrie build(String[] words) {
      var t = new ArenaTrie();
      for (int idx = 0; idx < words.length; idx++) {
        int node = 0;
        var w = words[idx];
        for (int i = 0; i < w.length(); i++) {
          int slot = node * STRIDE + 1 + w.charAt(i) - 'a';
          if (t.cells[slot] == 0) {
            int child = t.push();
            t.cells[slot] = child;
          }
          node = t.cells[slot];
        }
        t.cells[node * STRIDE] = idx;
      }
      return t;
    }

    void collect(int node, String[] words, List<String> out, int limit) {
      int base = node * STRIDE;
      if (cells[base] >= 0) {
        out.add(words[cells[base]]);
      }
      for (int c = 1; c <= LETTERS; c++) {
        if (out.size() >= limit) {
          return;
        }
        int child = cells[base + c];
        if (child != 0) {
          collect(child, words, out, limit);
        }
      }
    }

    List<String> all(String[] words) {
      var out = new ArrayList<String>(words.length);
      collect(0, words, out, words.length + 1);
      return out;
    }

    Completer completer(String[] words) {
      return (prefix, out) -> {
        int node = 0;
        for (int i = 0; i < prefix.length(); i++) {
          node = cells[node * STRIDE + 1 + prefix.charAt(i) - 'a'];
          if (node == 0) {
            return;
          }
        }
        collect(node, words, out, TOP_K);
      };
    }
  }
}
