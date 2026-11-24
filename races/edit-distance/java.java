// Race: edit-distance (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  private static final String ACGT = "acgt";

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

}
