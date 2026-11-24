package kernels;

import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.PriorityQueue;

/**
 * The string kernels of the puzzle group (kernels/SPEC.md section 13): palindrome, bwt,
 * aho-corasick and huffman. Puzzle.kernels() lists them with the rest of the group.
 */
final class PuzzleText {
  private static final int ONE_MIB = 1 << 20;

  private PuzzleText() {}

  // palindrome: Manacher's mirror trick finds every palindrome in linear time

  /** {@code a}/{@code b} from the bits of generator values, low bit first, 32 per value. */
  static byte[] binaryText(int n) {
    var rng = new Gen.XorShift32();
    var out = new byte[n];
    for (int i = 0; i < n; i += 32) {
      long v = rng.next();
      for (int j = i; j < Math.min(i + 32, n); j++) {
        out[j] = (byte) ((v & 1) != 0 ? 'b' : 'a');
        v >>= 1;
      }
    }
    return out;
  }

  static String palindrome(byte[] text) {
    var t = new byte[2 * text.length + 3];
    Arrays.fill(t, (byte) '#');
    t[0] = '^';
    t[t.length - 1] = '$';
    for (int k = 0; k < text.length; k++) {
      t[2 + 2 * k] = text[k];
    }
    var p = new int[t.length];
    int center = 0;
    int right = 0;
    for (int i = 1; i < t.length - 1; i++) {
      if (i < right) {
        p[i] = Math.min(right - i, p[2 * center - i]);
      }
      while (t[i + 1 + p[i]] == t[i - 1 - p[i]]) {
        p[i]++;
      }
      if (i + p[i] > right) {
        center = i;
        right = i + p[i];
      }
    }
    int longest = 0;
    int best = 0;
    long count = 0;
    for (int i = 0; i < p.length; i++) {
      count += (p[i] + 1) / 2;
      if (p[i] > longest) {
        longest = p[i];
        best = i;
      }
    }
    return text.length + ":" + longest + ":" + (best - 1 - longest) / 2 + ":" + count;
  }

  // bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

  private static final byte[] DNA = {'A', 'C', 'G', 'T'};

  /** {@code ACGT} from 2-bit chunks of generator values, low bits first, 16 per value. */
  static byte[] dnaText(int n) {
    var rng = new Gen.XorShift32();
    var out = new byte[n];
    for (int i = 0; i < n; i += 16) {
      long v = rng.next();
      for (int j = i; j < Math.min(i + 16, n); j++) {
        out[j] = DNA[(int) (v & 3)];
        v >>= 2;
      }
    }
    return out;
  }

  record BwtResult(int n, int rounds, int primary, byte[] out) {}

  static BwtResult bwt(byte[] dna) {
    var text = Arrays.copyOf(dna, dna.length + 1);
    text[dna.length] = '$';
    int n = text.length;
    var rank = new int[n];
    var sa = new Integer[n];
    for (int i = 0; i < n; i++) {
      rank[i] = text[i];
      sa[i] = i;
    }
    int k = 1;
    int rounds = 0;
    while (true) {
      rounds++;
      var second = new int[n];
      for (int i = 0; i < n; i++) {
        second[i] = i + k < n ? rank[i + k] : -1;
      }
      var first = rank;
      Arrays.sort(
          sa,
          (a, b) ->
              first[a] != first[b]
                  ? Integer.compare(first[a], first[b])
                  : Integer.compare(second[a], second[b]));
      var tmp = new int[n];
      for (int j = 1; j < n; j++) {
        int a = sa[j - 1];
        int b = sa[j];
        tmp[b] = tmp[a] + (rank[a] != rank[b] || second[a] != second[b] ? 1 : 0);
      }
      rank = tmp;
      if (rank[sa[n - 1]] == n - 1) {
        break;
      }
      k *= 2;
    }
    var out = new byte[n];
    int primary = 0;
    for (int j = 0; j < n; j++) {
      int i = sa[j];
      if (i == 0) {
        primary = j;
        out[j] = text[n - 1];
      } else {
        out[j] = text[i - 1];
      }
    }
    return new BwtResult(n, rounds, primary, out);
  }

  static String bwtChecksum(BwtResult r) {
    return r.n() + ":" + r.rounds() + ":" + r.primary() + ":" + Gen.fnv1a(r.out());
  }

  // aho-corasick: a trie with failure links matches thousands of patterns in one pass

  private static final int AHO_PATTERNS = 10000;

  record AhoInput(byte[][] patterns, byte[] text) {}

  static AhoInput ahoInput(int n) {
    var rng = new Gen.XorShift32();
    var patterns = new byte[AHO_PATTERNS][];
    for (int p = 0; p < AHO_PATTERNS; p++) {
      var pattern = new byte[6 + (int) (rng.next() % 7)];
      for (int i = 0; i < pattern.length; i++) {
        pattern[i] = DNA[(int) (rng.next() % 4)];
      }
      patterns[p] = pattern;
    }
    var block = new byte[ONE_MIB];
    for (int i = 0; i < ONE_MIB; i += 16) {
      long v = rng.next();
      for (int j = i; j < i + 16; j++) {
        block[j] = DNA[(int) (v & 3)];
        v >>= 2;
      }
    }
    var text = new byte[n / ONE_MIB * ONE_MIB];
    for (int offset = 0; offset < text.length; offset += ONE_MIB) {
      System.arraycopy(block, 0, text, offset, ONE_MIB);
    }
    return new AhoInput(patterns, text);
  }

  static String ahoCorasick(AhoInput in) {
    var code = new int[256];
    for (int i = 0; i < DNA.length; i++) {
      code[DNA[i]] = i;
    }
    var next = new int[4 * 1024];
    Arrays.fill(next, 0, 4, -1);
    var out = new int[1024];
    int states = 1;
    for (byte[] pattern : in.patterns()) {
      int s = 0;
      for (byte ch : pattern) {
        int edge = s * 4 + code[ch];
        if (next[edge] == -1) {
          if (states == out.length) {
            out = Arrays.copyOf(out, states * 2);
            next = Arrays.copyOf(next, states * 2 * 4);
          }
          next[edge] = states;
          Arrays.fill(next, states * 4, states * 4 + 4, -1);
          states++;
        }
        s = next[edge];
      }
      out[s]++;
    }
    var fail = new int[states];
    var queue = new int[states];
    int tail = 0;
    for (int c = 0; c < 4; c++) {
      if (next[c] == -1) {
        next[c] = 0;
      } else {
        queue[tail++] = next[c];
      }
    }
    for (int head = 0; head < tail; head++) {
      int s = queue[head];
      out[s] += out[fail[s]];
      for (int c = 0; c < 4; c++) {
        int t = next[s * 4 + c];
        if (t == -1) {
          next[s * 4 + c] = next[fail[s] * 4 + c];
        } else {
          fail[t] = next[fail[s] * 4 + c];
          queue[tail++] = t;
        }
      }
    }
    int s = 0;
    long matches = 0;
    long hits = 0;
    for (byte ch : in.text()) {
      s = next[s * 4 + code[ch]];
      int found = out[s];
      if (found > 0) {
        matches += found;
        hits++;
      }
    }
    return in.patterns().length + ":" + states + ":" + matches + ":" + hits;
  }

  // huffman: merge the two rarest symbols until one tree is left

  /** A 1 MiB block of letters skewed toward {@code a} (two generator values each), repeated. */
  static byte[] skewedText(int n) {
    var rng = new Gen.XorShift32();
    var block = new byte[ONE_MIB];
    for (int i = 0; i < ONE_MIB; i++) {
      long a = rng.next() % 26;
      long b = rng.next() % 26;
      block[i] = (byte) (97 + a * b / 26);
    }
    var text = new byte[n / ONE_MIB * ONE_MIB];
    for (int offset = 0; offset < text.length; offset += ONE_MIB) {
      System.arraycopy(block, 0, text, offset, ONE_MIB);
    }
    return text;
  }

  record HuffmanResult(int n, long bits, byte[] encoded, boolean ok) {}

  static HuffmanResult huffman(byte[] text) {
    var freq = new long[256];
    for (byte b : text) {
      freq[b & 0xFF]++;
    }
    var heap = new PriorityQueue<Long>();
    for (int sym = 0; sym < 256; sym++) {
      if (freq[sym] > 0) {
        heap.add(freq[sym] * 1024 + sym);
      }
    }
    var left = new int[512];
    var right = new int[512];
    int nextId = 256;
    while (heap.size() > 1) {
      long a = heap.poll();
      long b = heap.poll();
      left[nextId] = (int) (a % 1024);
      right[nextId] = (int) (b % 1024);
      heap.add((a / 1024 + b / 1024) * 1024 + nextId);
      nextId++;
    }
    int root = (int) (heap.peek() % 1024);
    var codes = new int[256];
    var lengths = new int[256];
    var stack = new ArrayDeque<int[]>();
    stack.push(new int[] {root, 0, 0});
    while (!stack.isEmpty()) {
      int[] top = stack.pop();
      int node = top[0];
      if (node < 256) {
        codes[node] = top[1];
        lengths[node] = top[2];
      } else {
        stack.push(new int[] {left[node], top[1] << 1, top[2] + 1});
        stack.push(new int[] {right[node], (top[1] << 1) | 1, top[2] + 1});
      }
    }
    long capacity = 0;
    for (int sym = 0; sym < 256; sym++) {
      capacity += freq[sym] * lengths[sym];
    }
    var encoded = new byte[(int) ((capacity + 7) / 8)];
    int written = 0;
    long acc = 0;
    int nbits = 0;
    long bits = 0;
    for (byte value : text) {
      int b = value & 0xFF;
      acc = (acc << lengths[b]) | codes[b];
      nbits += lengths[b];
      bits += lengths[b];
      while (nbits >= 8) {
        nbits -= 8;
        encoded[written++] = (byte) (acc >> nbits);
      }
      acc &= (1L << nbits) - 1;
    }
    if (nbits > 0) {
      encoded[written++] = (byte) (acc << (8 - nbits));
    }
    var decoded = new byte[text.length];
    int node = root;
    int k = 0;
    for (int i = 0; i < written && k < text.length; i++) {
      int byteValue = encoded[i] & 0xFF;
      for (int shift = 7; shift >= 0 && k < text.length; shift--) {
        node = ((byteValue >> shift) & 1) != 0 ? right[node] : left[node];
        if (node < 256) {
          decoded[k++] = (byte) node;
          node = root;
        }
      }
    }
    return new HuffmanResult(text.length, bits, encoded, Arrays.equals(decoded, text));
  }

  static String huffmanChecksum(HuffmanResult r) {
    return r.n() + ":" + r.bits() + ":" + Gen.fnv1a(r.encoded()) + ":" + (r.ok() ? 1 : 0);
  }
}
