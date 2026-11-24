package kernels;

import java.util.Arrays;
import java.util.Map;

/** Classic algorithm kernels (kernels/SPEC.md section 12, group algo). */
final class Algo {
  private static final int KNAPSACK_ITEMS = 1000;
  private static final int PATTERN_LENGTH = 8;
  private static final String ACGT = "acgt";

  /** The 64 most common English words (kernels/SPEC.md section 12, kmp-en). */
  private static final String[] WORDS =
      ("the be to of and a in that have i it for not on with he as you do at this but his by from"
              + " they we say her she or an will my one all would there their what so up out if about"
              + " who get which go me when make can like time no just him know take people into year"
              + " your")
          .split(" ");

  private Algo() {}

  static Map<String, Main.Kernel<?, ?>> kernels() {
    return Map.ofEntries(
        Map.entry(
            "bfs-grid",
            new Main.Kernel<>(
                (n, data) -> AlgoGraphs.grid(n),
                g -> g,
                (g, ctx) -> AlgoGraphs.bfsGrid(g),
                s -> s)),
        Map.entry(
            "dijkstra",
            new Main.Kernel<>(
                (n, data) -> AlgoGraphs.graph(n),
                g -> g,
                (g, ctx) -> AlgoGraphs.dijkstraCounted(g),
                AlgoGraphs.Counted::checksum,
                AlgoGraphs.Counted::extra)),
        Map.entry(
            "kruskal",
            new Main.Kernel<>(
                (n, data) -> AlgoGraphs.edgeList(n),
                AlgoGraphs.EdgeList::copy,
                (e, ctx) -> AlgoGraphs.kruskal(e),
                s -> s)),
        Map.entry(
            "bst",
            new Main.Kernel<>(
                (n, data) -> Misc.hashmapInput(n), m -> m, (m, ctx) -> AlgoTrees.bst(m), s -> s)),
        Map.entry(
            "ordered-map",
            new Main.Kernel<>(
                (n, data) -> Misc.hashmapInput(n),
                m -> m,
                (m, ctx) -> AlgoTrees.orderedMap(m),
                s -> s)),
        Map.entry(
            "trie",
            new Main.Kernel<>(
                (n, data) -> AlgoTrees.words(n), w -> w, (w, ctx) -> AlgoTrees.trie(w), s -> s)),
        Map.entry(
            "lru",
            new Main.Kernel<>(
                (n, data) -> AlgoTrees.lruKeys(n), k -> k, (k, ctx) -> AlgoTrees.lru(k), s -> s)),
        Map.entry(
            "edit-distance",
            new Main.Kernel<>(
                (n, data) -> dna(n, 2), ab -> ab, (ab, ctx) -> editDistance(ab[0], ab[1]), s -> s)),
        Map.entry(
            "knapsack",
            new Main.Kernel<>((n, data) -> items(n), k -> k, (k, ctx) -> knapsack(k), s -> s)),
        Map.entry(
            "lis", new Main.Kernel<>((n, data) -> Gen.ints(n), a -> a, (a, ctx) -> lis(a), s -> s)),
        Map.entry(
            "kmp", new Main.Kernel<>((n, data) -> text(n), t -> t, (t, ctx) -> kmp(t), s -> s)),
        Map.entry(
            "str-find",
            new Main.Kernel<>((n, data) -> text(n), t -> t, (t, ctx) -> strFind(t), s -> s)),
        Map.entry(
            "kmp-en",
            new Main.Kernel<>((n, data) -> textEn(n), t -> t, (t, ctx) -> kmp(t), s -> s)),
        Map.entry(
            "str-find-en",
            new Main.Kernel<>((n, data) -> textEn(n), t -> t, (t, ctx) -> strFind(t), s -> s)),
        Map.entry("fib", Main.Kernel.sized((n, ctx) -> Long.toString(fib(n)), s -> s)),
        Map.entry("nqueens", Main.Kernel.sized((n, ctx) -> Long.toString(nqueens(n)), s -> s)),
        Map.entry(
            "binary-search",
            new Main.Kernel<>(
                (n, data) -> sortedArray(n), s -> s, (s, ctx) -> binarySearch(s), s -> s)));
  }

  static String[] dna(int length, int count) {
    var rng = new Gen.XorShift32();
    var out = new String[count];
    for (int s = 0; s < count; s++) {
      var chars = new char[length];
      for (int i = 0; i < length; i++) {
        chars[i] = ACGT.charAt((int) (rng.next() % 4));
      }
      out[s] = new String(chars);
    }
    return out;
  }

  static String editDistance(String a, String b) {
    var prev = new int[b.length() + 1];
    var cur = new int[b.length() + 1];
    for (int j = 0; j <= b.length(); j++) {
      prev[j] = j;
    }
    for (int i = 0; i < a.length(); i++) {
      cur[0] = i + 1;
      char ca = a.charAt(i);
      for (int j = 0; j < b.length(); j++) {
        int cost = ca == b.charAt(j) ? 0 : 1;
        cur[j + 1] = Math.min(Math.min(prev[j + 1] + 1, cur[j] + 1), prev[j] + cost);
      }
      var swap = prev;
      prev = cur;
      cur = swap;
    }
    return Integer.toString(prev[b.length()]);
  }

  record Knapsack(int capacity, int[] weights, long[] values) {}

  static Knapsack items(int capacity) {
    var rng = new Gen.XorShift32();
    var weights = new int[KNAPSACK_ITEMS];
    var values = new long[KNAPSACK_ITEMS];
    for (int i = 0; i < KNAPSACK_ITEMS; i++) {
      weights[i] = 1 + (int) (rng.next() % 1000);
      values[i] = 1 + rng.next() % 1000;
    }
    return new Knapsack(capacity, weights, values);
  }

  static String knapsack(Knapsack k) {
    int capacity = k.capacity();
    var best = new long[capacity + 1];
    for (int i = 0; i < k.weights().length; i++) {
      int weight = k.weights()[i];
      long value = k.values()[i];
      for (int c = capacity; c >= weight; c--) {
        if (best[c - weight] + value > best[c]) {
          best[c] = best[c - weight] + value;
        }
      }
    }
    return best[capacity] + ":" + Gen.h31(best);
  }

  static String lis(long[] values) {
    var tails = new long[16];
    int length = 0;
    for (long x : values) {
      int i = Arrays.binarySearch(tails, 0, length, x);
      if (i < 0) {
        i = -(i + 1);
      }
      if (i == tails.length) {
        tails = Arrays.copyOf(tails, tails.length * 2);
      }
      tails[i] = x;
      if (i == length) {
        length++;
      }
    }
    return length + ":" + Gen.h31(Arrays.copyOf(tails, length));
  }

  record Text(String text, String pattern) {}

  static Text text(int n) {
    var t = dna(n, 1)[0];
    return new Text(t, t.substring(n / 2, n / 2 + PATTERN_LENGTH));
  }

  static String english(int n) {
    var rng = new Gen.XorShift32();
    var text = new StringBuilder(n + 16);
    while (text.length() < n) {
      long a = rng.next() % 64;
      long b = rng.next() % 64;
      text.append(WORDS[(int) Math.min(a, b)]);
      long r = rng.next() % 16;
      text.append(r == 0 ? ". " : r == 1 ? ", " : " ");
    }
    return text.substring(0, n);
  }

  static Text textEn(int n) {
    var t = english(n);
    return new Text(t, t.substring(n / 2, n / 2 + PATTERN_LENGTH));
  }

  static String kmp(Text in) {
    var t = in.text();
    var p = in.pattern();
    int m = p.length();
    var fail = new int[m];
    int k = 0;
    for (int i = 1; i < m; i++) {
      while (k > 0 && p.charAt(i) != p.charAt(k)) {
        k = fail[k - 1];
      }
      if (p.charAt(i) == p.charAt(k)) {
        k++;
      }
      fail[i] = k;
    }
    int count = 0;
    int first = -1;
    int last = -1;
    k = 0;
    for (int i = 0; i < t.length(); i++) {
      char ch = t.charAt(i);
      while (k > 0 && ch != p.charAt(k)) {
        k = fail[k - 1];
      }
      if (ch == p.charAt(k)) {
        k++;
      }
      if (k == m) {
        last = i + 1 - m;
        if (first < 0) {
          first = last;
        }
        count++;
        k = fail[k - 1];
      }
    }
    return count + ":" + first + ":" + last;
  }

  static String strFind(Text in) {
    int count = 0;
    int first = -1;
    int last = -1;
    int pos = in.text().indexOf(in.pattern());
    while (pos >= 0) {
      if (first < 0) {
        first = pos;
      }
      last = pos;
      count++;
      pos = in.text().indexOf(in.pattern(), pos + 1);
    }
    return count + ":" + first + ":" + last;
  }

  static long fib(int n) {
    return n < 2 ? n : fib(n - 1) + fib(n - 2);
  }

  static long nqueens(int n) {
    return new Board(n).place(0);
  }

  private static final class Board {
    private final int n;
    private final boolean[] cols;
    private final boolean[] up;
    private final boolean[] down;

    Board(int n) {
      this.n = n;
      cols = new boolean[n];
      up = new boolean[2 * n - 1];
      down = new boolean[2 * n - 1];
    }

    long place(int row) {
      if (row == n) {
        return 1;
      }
      long count = 0;
      for (int col = 0; col < n; col++) {
        int u = row + col;
        int d = row + n - 1 - col;
        if (!cols[col] && !up[u] && !down[d]) {
          cols[col] = up[u] = down[d] = true;
          count += place(row + 1);
          cols[col] = up[u] = down[d] = false;
        }
      }
      return count;
    }
  }

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
