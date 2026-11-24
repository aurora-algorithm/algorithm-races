// Race: bloom (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

import java.util.Arrays;

final class Puzzle {

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

}
