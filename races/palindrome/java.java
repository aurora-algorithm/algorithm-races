// Race: palindrome (java)
// Extracted from suite/java/src/main/java/kernels/PuzzleText.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.PriorityQueue;

import java.util.Arrays;

final class PuzzleText {

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

}
