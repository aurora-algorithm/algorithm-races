// Race: aho-corasick (java)
// Extracted from suite/java/src/main/java/kernels/PuzzleText.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.PriorityQueue;

import java.util.Arrays;

final class PuzzleText {

  private static final int ONE_MIB = 1 << 20;

  private static final byte[] DNA = {'A', 'C', 'G', 'T'};

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

}
