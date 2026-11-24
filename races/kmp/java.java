// Race: kmp (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  private static final int PATTERN_LENGTH = 8;

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

  record Text(String text, String pattern) {}

  static Text text(int n) {
    var t = dna(n, 1)[0];
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

}
