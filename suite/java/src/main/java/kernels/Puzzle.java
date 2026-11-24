package kernels;

import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

/**
 * Unusual algorithm kernels (kernels/SPEC.md section 13): one clever trick each, the same steps in
 * every language. Plain Java with no reflection, so native-image compiles it like the core kernels.
 */
final class Puzzle {
  private static final long MASK = 0xFFFFFFFFL;

  private Puzzle() {}

  /**
   * MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
   */
  static int fmix32(int h) {
    h ^= h >>> 16;
    h *= 0x85ebca6b;
    h ^= h >>> 13;
    h *= 0xc2b2ae35;
    h ^= h >>> 16;
    return h;
  }

  static void shuffle(Gen.XorShift32 rng, int[] items) {
    for (int i = items.length - 1; i > 0; i--) {
      int j = (int) (rng.next() % (i + 1));
      int t = items[i];
      items[i] = items[j];
      items[j] = t;
    }
  }

  // sudoku: Knuth's Algorithm X with dancing links

  private static final int SUDOKU_CLUES = 26;
  private static final int DLX_COLUMNS = 324;

  static int[][] sudokuPuzzles(int n) {
    var rng = new Gen.XorShift32();
    var puzzles = new int[n][];
    for (int p = 0; p < n; p++) {
      int[] digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      shuffle(rng, digits);
      var order = new int[2][9];
      for (int axis = 0; axis < 2; axis++) {
        int[] bands = {0, 1, 2};
        shuffle(rng, bands);
        int line = 0;
        for (int band : bands) {
          int[] inner = {0, 1, 2};
          shuffle(rng, inner);
          for (int i : inner) {
            order[axis][line++] = band * 3 + i;
          }
        }
      }
      int[] rows = order[0];
      int[] cols = order[1];
      var cells = new int[81];
      for (int i = 0; i < 81; i++) {
        cells[i] = i;
      }
      shuffle(rng, cells);
      var puzzle = new int[81];
      for (int k = 0; k < SUDOKU_CLUES; k++) {
        int cell = cells[k];
        int r = rows[cell / 9];
        int c = cols[cell % 9];
        puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
      }
      puzzles[p] = puzzle;
    }
    return puzzles;
  }

  /** The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the headers, then rows. */
  static final class Dlx {
    final int[] left;
    final int[] right;
    final int[] up;
    final int[] down;
    final int[] col;
    final int[] row;
    final int[] count = new int[DLX_COLUMNS + 1];
    final int[] chosen = new int[81];
    int[] first = new int[0];
    long solutions;
    long tries;

    Dlx() {
      int size = 1 + DLX_COLUMNS + 729 * 4;
      left = new int[size];
      right = new int[size];
      up = new int[size];
      down = new int[size];
      col = new int[size];
      row = new int[size];
      for (int i = 0; i < size; i++) {
        up[i] = i;
        down[i] = i;
        col[i] = i;
      }
      for (int i = 0; i <= DLX_COLUMNS; i++) {
        left[i] = i - 1;
        right[i] = i + 1;
      }
      left[0] = DLX_COLUMNS;
      right[DLX_COLUMNS] = 0;
      int node = DLX_COLUMNS + 1;
      for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
          int box = (r / 3) * 3 + c / 3;
          for (int d = 0; d < 9; d++) {
            int first = node;
            int[] columns = {1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d, 244 + box * 9 + d};
            for (int column : columns) {
              col[node] = column;
              row[node] = (r * 9 + c) * 9 + d;
              up[node] = up[column];
              down[node] = column;
              down[up[column]] = node;
              up[column] = node;
              count[column]++;
              left[node] = node - 1;
              right[node] = node + 1;
              node++;
            }
            left[first] = node - 1;
            right[node - 1] = first;
          }
        }
      }
    }

    void cover(int c) {
      right[left[c]] = right[c];
      left[right[c]] = left[c];
      for (int i = down[c]; i != c; i = down[i]) {
        for (int j = right[i]; j != i; j = right[j]) {
          down[up[j]] = down[j];
          up[down[j]] = up[j];
          count[col[j]]--;
        }
      }
    }

    void uncover(int c) {
      for (int i = up[c]; i != c; i = up[i]) {
        for (int j = left[i]; j != i; j = left[j]) {
          count[col[j]]++;
          down[up[j]] = j;
          up[down[j]] = j;
        }
      }
      right[left[c]] = c;
      left[right[c]] = c;
    }

    void chooseRow(int node) {
      for (int j = right[node]; j != node; j = right[j]) {
        cover(col[j]);
      }
    }

    void unchooseRow(int node) {
      for (int j = left[node]; j != node; j = left[j]) {
        uncover(col[j]);
      }
    }

    void search(int depth) {
      if (right[0] == 0) {
        solutions++;
        if (solutions == 1) {
          first = Arrays.copyOf(chosen, depth);
        }
        return;
      }
      int best = right[0];
      for (int c = right[best]; c != 0; c = right[c]) {
        if (count[c] < count[best]) {
          best = c;
        }
      }
      cover(best);
      for (int r = down[best]; r != best; r = down[r]) {
        tries++;
        chosen[depth] = row[r];
        chooseRow(r);
        search(depth + 1);
        unchooseRow(r);
      }
      uncover(best);
    }
  }

  static String sudoku(int[][] puzzles) {
    long solutions = 0;
    long tries = 0;
    long h = 0;
    for (int[] puzzle : puzzles) {
      var dlx = new Dlx();
      int depth = 0;
      for (int cell = 0; cell < 81; cell++) {
        int digit = puzzle[cell];
        if (digit != 0) {
          int node = DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4;
          dlx.cover(dlx.col[node]);
          dlx.chooseRow(node);
          dlx.chosen[depth++] = cell * 9 + digit - 1;
        }
      }
      dlx.search(depth);
      solutions += dlx.solutions;
      tries += dlx.tries;
      var chain = new long[82];
      chain[0] = h;
      for (int row : dlx.first) {
        chain[1 + row / 9] = row % 9 + 1;
      }
      h = Gen.h31(chain);
    }
    return puzzles.length + ":" + solutions + ":" + tries + ":" + h;
  }

  // bloom: k bit positions per key from two hashes

  private static final int BLOOM_BITS_PER_KEY = 10;
  private static final int BLOOM_HASHES = 7;
  private static final int GOLDEN = 0x9e3779b9;

  record BloomInput(long[] keys, long[] lookups) {}

  static BloomInput bloomInput(int n) {
    var values = Gen.ints(2 * n);
    return new BloomInput(Arrays.copyOfRange(values, 0, n), Arrays.copyOfRange(values, n, 2 * n));
  }

  static String bloom(BloomInput in) {
    int m = BLOOM_BITS_PER_KEY * in.keys().length;
    var bits = new byte[(m + 7) / 8];
    for (long key : in.keys()) {
      int h1 = fmix32((int) key);
      int h2 = fmix32((int) key ^ GOLDEN) | 1;
      for (int i = 0; i < BLOOM_HASHES; i++) {
        int b = Integer.remainderUnsigned(h1 + i * h2, m);
        bits[b >> 3] |= (byte) (1 << (b & 7));
      }
    }
    long falsePositives = 0;
    for (long key : in.lookups()) {
      int h1 = fmix32((int) key);
      int h2 = fmix32((int) key ^ GOLDEN) | 1;
      boolean all = true;
      for (int i = 0; i < BLOOM_HASHES; i++) {
        int b = Integer.remainderUnsigned(h1 + i * h2, m);
        if ((bits[b >> 3] & (1 << (b & 7))) == 0) {
          all = false;
          break;
        }
      }
      if (all) {
        falsePositives++;
      }
    }
    long setBits = 0;
    for (byte b : bits) {
      setBits += Integer.bitCount(b & 0xFF);
    }
    return in.keys().length + ":" + setBits + ":" + falsePositives;
  }

  // hyperloglog: the longest run of leading zeros estimates how many distinct items went by

  private static final int HLL_P = 14;
  private static final int HLL_M = 1 << HLL_P;

  static String hyperloglog(int n) {
    var rng = new Gen.XorShift32();
    int universe = Math.max(n / 4, 1);
    var registers = new byte[HLL_M];
    for (int k = 0; k < n; k++) {
      int h = fmix32(Integer.remainderUnsigned((int) rng.next(), universe));
      int w = h << HLL_P;
      int rank = w != 0 ? Integer.numberOfLeadingZeros(w) + 1 : 32 - HLL_P + 1;
      int idx = h >>> (32 - HLL_P);
      if (rank > registers[idx]) {
        registers[idx] = (byte) rank;
      }
    }
    double z = 0.0;
    int zeros = 0;
    var values = new long[HLL_M];
    for (int i = 0; i < HLL_M; i++) {
      int r = registers[i];
      z += Math.pow(2.0, -r);
      if (r == 0) {
        zeros++;
      }
      values[i] = r;
    }
    double m = HLL_M;
    double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if (estimate <= 2.5 * m && zeros > 0) {
      estimate = m * Math.log(m / zeros);
    }
    return n + ":" + (long) Math.floor(estimate + 0.5) + ":" + Gen.h31(values);
  }

  // skiplist: towers of random height turn a linked list into a search tree

  private static final int SKIP_MAX_LEVEL = 24;
  private static final long SKIP_LEVEL_SEED = 88675123L;

  static final class SkipNode {
    final long key;
    final SkipNode[] next;

    SkipNode(long key, int height) {
      this.key = key;
      this.next = new SkipNode[height];
    }
  }

  record SkipInput(long[] keys, long[] lookups) {}

  record SkipResult(SkipNode head, int n, long hits, long heights) {}

  static SkipInput skiplistInput(int n) {
    var keys = Gen.ints(n);
    var lookups = new long[n];
    for (int i = 0; i < n; i++) {
      lookups[i] = keys[i] ^ (i & 1);
    }
    return new SkipInput(keys, lookups);
  }

  static SkipResult skiplist(SkipInput in) {
    var rng = new Gen.XorShift32(SKIP_LEVEL_SEED);
    var head = new SkipNode(-1, SKIP_MAX_LEVEL);
    int level = 1;
    var update = new SkipNode[SKIP_MAX_LEVEL];
    Arrays.fill(update, head);
    long heights = 0;
    for (long key : in.keys()) {
      var x = head;
      for (int i = level - 1; i >= 0; i--) {
        while (x.next[i] != null && x.next[i].key < key) {
          x = x.next[i];
        }
        update[i] = x;
      }
      if (x.next[0] != null && x.next[0].key == key) {
        continue;
      }
      int height = 1;
      while (height < SKIP_MAX_LEVEL && (rng.next() & 1) != 0) {
        height++;
      }
      for (int i = level; i < height; i++) {
        update[i] = head;
      }
      level = Math.max(level, height);
      var node = new SkipNode(key, height);
      for (int i = 0; i < height; i++) {
        node.next[i] = update[i].next[i];
        update[i].next[i] = node;
      }
      heights += height;
    }
    long hits = 0;
    for (long key : in.lookups()) {
      var x = head;
      for (int i = level - 1; i >= 0; i--) {
        while (x.next[i] != null && x.next[i].key < key) {
          x = x.next[i];
        }
      }
      if (x.next[0] != null && x.next[0].key == key) {
        hits++;
      }
    }
    return new SkipResult(head, in.keys().length, hits, heights);
  }

  static String skiplistChecksum(SkipResult result) {
    long h = 0;
    for (var x = result.head().next[0]; x != null; x = x.next[0]) {
      h = (h * 31 + x.key) & MASK;
    }
    return result.n() + ":" + result.hits() + ":" + result.heights() + ":" + h;
  }

  // cycle: Floyd's tortoise and hare on x -> x*x + c mod M

  private static final long CYCLE_MOD = 67108859L;
  private static final long CYCLE_C = 12345L;

  static long[] cycleStarts(int n) {
    var rng = new Gen.XorShift32();
    var starts = new long[n];
    for (int i = 0; i < n; i++) {
      starts[i] = rng.next() % CYCLE_MOD;
    }
    return starts;
  }

  static long step(long x) {
    return (x * x + CYCLE_C) % CYCLE_MOD;
  }

  static String cycle(long[] starts) {
    long totalMu = 0;
    long totalLam = 0;
    long longest = 0;
    for (long x0 : starts) {
      long tortoise = step(x0);
      long hare = step(tortoise);
      while (tortoise != hare) {
        tortoise = step(tortoise);
        hare = step(step(hare));
      }
      long mu = 0;
      tortoise = x0;
      while (tortoise != hare) {
        tortoise = step(tortoise);
        hare = step(hare);
        mu++;
      }
      long lam = 1;
      hare = step(tortoise);
      while (tortoise != hare) {
        hare = step(hare);
        lam++;
      }
      totalMu += mu;
      totalLam += lam;
      longest = Math.max(longest, mu + lam);
    }
    return starts.length + ":" + totalMu + ":" + totalLam + ":" + longest;
  }

  // pi-digits: the Chudnovsky series, summed exactly by binary splitting

  private static final long CHUD_A = 13591409L;
  private static final long CHUD_B = 545140134L;
  private static final BigInteger CHUD_C3_24 = BigInteger.valueOf(10939058860032000L);
  private static final int DIGITS_PER_TERM = 14;
  private static final int GUARD_DIGITS = 10;

  /** P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1. */
  record Split(BigInteger p, BigInteger q, BigInteger t) {}

  static Split binarySplit(long a, long b) {
    if (b - a == 1) {
      BigInteger p;
      BigInteger q;
      if (a == 0) {
        p = BigInteger.ONE;
        q = BigInteger.ONE;
      } else {
        p = BigInteger.valueOf((6 * a - 5) * (2 * a - 1) * (6 * a - 1));
        q = BigInteger.valueOf(a * a * a).multiply(CHUD_C3_24);
      }
      var t = p.multiply(BigInteger.valueOf(CHUD_A + CHUD_B * a));
      return new Split(p, q, a % 2 == 1 ? t.negate() : t);
    }
    long m = (a + b) / 2;
    var left = binarySplit(a, m);
    var right = binarySplit(m, b);
    return new Split(
        left.p().multiply(right.p()),
        left.q().multiply(right.q()),
        left.t().multiply(right.q()).add(left.p().multiply(right.t())));
  }

  /**
   * Floor square root of n > 0 by precision doubling (CPython's math.isqrt), written out as in the
   * others.
   */
  static BigInteger isqrt(BigInteger n) {
    int c = (n.bitLength() - 1) / 2;
    var a = BigInteger.ONE;
    int d = 0;
    for (int s = 31 - Integer.numberOfLeadingZeros(c); s >= 0; s--) {
      int e = d;
      d = c >> s;
      a = a.shiftLeft(d - e - 1).add(n.shiftRight(2 * c - e - d + 1).divide(a));
    }
    return a.multiply(a).compareTo(n) > 0 ? a.subtract(BigInteger.ONE) : a;
  }

  static String piDigits(int n) {
    long terms = n / DIGITS_PER_TERM + 2;
    var split = binarySplit(0, terms);
    var one = BigInteger.TEN.pow(n + GUARD_DIGITS);
    var sqrtC = isqrt(BigInteger.valueOf(10005).multiply(one).multiply(one));
    var pi = BigInteger.valueOf(426880).multiply(sqrtC).multiply(split.q()).divide(split.t());
    return pi.divide(BigInteger.TEN.pow(GUARD_DIGITS)).toString();
  }

  static String piChecksum(String digits) {
    long sum = 0;
    for (int i = 0; i < digits.length(); i++) {
      sum += digits.charAt(i) - '0';
    }
    return (digits.length() - 1) + ":" + digits.substring(digits.length() - 50) + ":" + sum;
  }

  // life: Conway's Game of Life on a torus

  private static final int LIFE_SIDE = 512;

  record LifeInput(byte[] grid, int gens) {}

  record LifeResult(int gens, byte[] grid) {}

  static LifeInput lifeInput(int gens) {
    var rng = new Gen.XorShift32();
    var grid = new byte[LIFE_SIDE * LIFE_SIDE];
    for (int i = 0; i < grid.length; i++) {
      grid[i] = (byte) (rng.next() % 4 == 0 ? 1 : 0);
    }
    return new LifeInput(grid, gens);
  }

  static LifeResult life(LifeInput in) {
    int w = LIFE_SIDE;
    var cur = in.grid().clone();
    var nxt = new byte[cur.length];
    for (int g = 0; g < in.gens(); g++) {
      for (int y = 0; y < w; y++) {
        int up = (y + w - 1) % w * w;
        int row = y * w;
        int down = (y + 1) % w * w;
        for (int x = 0; x < w; x++) {
          int left = (x + w - 1) % w;
          int right = (x + 1) % w;
          int n =
              cur[up + left]
                  + cur[up + x]
                  + cur[up + right]
                  + cur[row + left]
                  + cur[row + right]
                  + cur[down + left]
                  + cur[down + x]
                  + cur[down + right];
          nxt[row + x] = (byte) (n == 3 || (n == 2 && cur[row + x] != 0) ? 1 : 0);
        }
      }
      var t = cur;
      cur = nxt;
      nxt = t;
    }
    return new LifeResult(in.gens(), cur);
  }

  static String lifeChecksum(LifeResult r) {
    long alive = 0;
    for (byte b : r.grid()) {
      alive += b;
    }
    return r.gens() + ":" + alive + ":" + Gen.fnv1a(r.grid());
  }

  // window-max: a monotonic deque keeps the maximum of every window in O(1) per step

  private static final int[] WINDOWS = {10, 100, 1000, 10000, 100000};

  static String windowMax(long[] values) {
    int n = values.length;
    var dq = new int[n];
    var result = new StringBuilder().append(n);
    for (int w : WINDOWS) {
      int head = 0;
      int tail = 0;
      long h = 0;
      for (int i = 0; i < n; i++) {
        long v = values[i];
        while (tail > head && values[dq[tail - 1]] <= v) {
          tail--;
        }
        dq[tail++] = i;
        if (dq[head] <= i - w) {
          head++;
        }
        if (i >= w - 1) {
          h = (h * 31 + values[dq[head]]) & MASK;
        }
      }
      result.append(':').append(h);
    }
    return result.toString();
  }

  // rain-water: flood a 2D terrain from its walls inward with a min-heap

  static int[] terrain(int side) {
    var rng = new Gen.XorShift32();
    var heights = new int[side * side];
    for (int i = 0; i < heights.length; i++) {
      heights[i] = (int) (rng.next() % 1000);
    }
    return heights;
  }

  static String rainWater(int[] heights) {
    int n = heights.length;
    int side = (int) Math.sqrt(n);
    var seen = new boolean[n];
    var heap = new PriorityQueue<Long>();
    for (int idx = 0; idx < n; idx++) {
      int r = idx / side;
      int c = idx % side;
      if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
        seen[idx] = true;
        heap.add((long) heights[idx] * n + idx);
      }
    }
    long water = 0;
    long wet = 0;
    int[] dr = {-1, 1, 0, 0};
    int[] dc = {0, 0, -1, 1};
    while (!heap.isEmpty()) {
      long key = heap.poll();
      long level = key / n;
      int idx = (int) (key % n);
      int r = idx / side;
      int c = idx % side;
      for (int d = 0; d < 4; d++) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        if (nr >= 0 && nr < side && nc >= 0 && nc < side) {
          int nidx = nr * side + nc;
          if (!seen[nidx]) {
            seen[nidx] = true;
            long h = heights[nidx];
            if (h < level) {
              water += level - h;
              wet++;
              h = level;
            }
            heap.add(h * n + nidx);
          }
        }
      }
    }
    return side + ":" + water + ":" + wet;
  }

  // collatz: remember every chain length below the start so each walk stops early

  static String collatz(int n) {
    var cache = new int[n];
    cache[1] = 1;
    long best = 1;
    long bestLen = 1;
    long total = 1;
    for (int start = 2; start < n; start++) {
      long x = start;
      int steps = 0;
      while (x >= start) {
        x = x % 2 == 0 ? x / 2 : 3 * x + 1;
        steps++;
      }
      int length = steps + cache[(int) x];
      cache[start] = length;
      total += length;
      if (length > bestLen) {
        best = start;
        bestLen = length;
      }
    }
    return n + ":" + best + ":" + bestLen + ":" + total;
  }

  /** Every input here is read-only inside the kernel, so each repeat reuses it. */
  private static <I, R> Main.Kernel<I, R> kernel(
      Main.Prepare<I> prepare, Function<I, R> run, Function<R, String> checksum) {
    return new Main.Kernel<>(prepare, in -> in, (in, ctx) -> run.apply(in), checksum);
  }

  static Map<String, Main.Kernel<?, ?>> kernels() {
    return Map.ofEntries(
        Map.entry("sudoku", kernel((n, data) -> sudokuPuzzles(n), Puzzle::sudoku, s -> s)),
        Map.entry("bloom", kernel((n, data) -> bloomInput(n), Puzzle::bloom, s -> s)),
        Map.entry("hyperloglog", Main.Kernel.sized((n, ctx) -> hyperloglog(n), s -> s)),
        Map.entry(
            "skiplist",
            kernel((n, data) -> skiplistInput(n), Puzzle::skiplist, Puzzle::skiplistChecksum)),
        Map.entry("cycle", kernel((n, data) -> cycleStarts(n), Puzzle::cycle, s -> s)),
        Map.entry(
            "palindrome",
            kernel((n, data) -> PuzzleText.binaryText(n), PuzzleText::palindrome, s -> s)),
        Map.entry(
            "bwt",
            kernel((n, data) -> PuzzleText.dnaText(n), PuzzleText::bwt, PuzzleText::bwtChecksum)),
        Map.entry(
            "aho-corasick",
            kernel((n, data) -> PuzzleText.ahoInput(n), PuzzleText::ahoCorasick, s -> s)),
        Map.entry("pi-digits", Main.Kernel.sized((n, ctx) -> piDigits(n), Puzzle::piChecksum)),
        Map.entry("life", kernel((n, data) -> lifeInput(n), Puzzle::life, Puzzle::lifeChecksum)),
        Map.entry(
            "huffman",
            kernel(
                (n, data) -> PuzzleText.skewedText(n),
                PuzzleText::huffman,
                PuzzleText::huffmanChecksum)),
        Map.entry("window-max", kernel((n, data) -> Gen.ints(n), Puzzle::windowMax, s -> s)),
        Map.entry("rain-water", kernel((n, data) -> terrain(n), Puzzle::rainWater, s -> s)),
        Map.entry("collatz", Main.Kernel.sized((n, ctx) -> collatz(n), s -> s)));
  }
}
