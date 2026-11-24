// Race: bwt (java)
// Extracted from suite/java/src/main/java/kernels/PuzzleText.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.PriorityQueue;

import java.util.Arrays;

final class PuzzleText {

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

}
