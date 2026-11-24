// Race: pi-digits (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

import java.math.BigInteger;

final class Puzzle {

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

}
