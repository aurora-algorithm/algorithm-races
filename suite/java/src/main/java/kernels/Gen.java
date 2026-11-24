package kernels;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

/** Shared inputs and checksums (kernels/SPEC.md section 2). Every language implements these. */
final class Gen {
  private static final long MASK = 0xFFFFFFFFL;
  private static final int FNV_OFFSET = 0x811C9DC5;
  private static final int FNV_PRIME = 16777619;
  private static final int ONE_MIB = 1 << 20;
  private static final String[] METHODS = {"GET", "POST", "PUT", "DELETE"};
  private static final int[] STATUSES = {
    200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503
  };

  private Gen() {}

  /** Marsaglia's xorshift32: values 1 .. 2^32-1, returned as non-negative longs. */
  static final class XorShift32 {
    private int state;

    XorShift32() {
      this(2463534242L);
    }

    XorShift32(long seed) {
      state = (int) seed;
    }

    long next() {
      state ^= state << 13;
      state ^= state >>> 17;
      state ^= state << 5;
      return Integer.toUnsignedLong(state);
    }

    long[] ints(int n) {
      var out = new long[n];
      for (int i = 0; i < n; i++) {
        out[i] = next();
      }
      return out;
    }
  }

  static long[] ints(int n) {
    return new XorShift32().ints(n);
  }

  static String[] strings(int n) {
    var rng = new XorShift32();
    var out = new String[n];
    for (int i = 0; i < n; i++) {
      var chars = new char[8 + (int) (rng.next() % 9)];
      for (int c = 0; c < chars.length; c++) {
        chars[c] = (char) ('a' + rng.next() % 26);
      }
      out[i] = new String(chars);
    }
    return out;
  }

  /** One 1 MiB block of generator values (4 bytes each, little-endian), repeated. */
  static byte[] shaBuffer(int size) {
    var block = ByteBuffer.allocate(ONE_MIB).order(ByteOrder.LITTLE_ENDIAN);
    for (long v : ints(ONE_MIB / 4)) {
      block.putInt((int) v);
    }
    var out = new byte[size];
    for (int offset = 0; offset < size; offset += ONE_MIB) {
      System.arraycopy(block.array(), 0, out, offset, ONE_MIB);
    }
    return out;
  }

  static String logText(int lines) {
    var rng = new XorShift32();
    var text = new StringBuilder(lines * 36);
    for (int i = 0; i < lines; i++) {
      var method = METHODS[(int) (rng.next() % 4)];
      var version = 1 + rng.next() % 3;
      var user = rng.next() % 100000;
      var status = STATUSES[(int) (rng.next() % 12)];
      var ms = rng.next() % 1000;
      text.append(method)
          .append(" /api/v")
          .append(version)
          .append("/users/")
          .append(user)
          .append(' ')
          .append(status)
          .append(' ')
          .append(ms)
          .append("ms\n");
    }
    return text.toString();
  }

  record Matrices(double[] a, double[] b, int n) {}

  static Matrices matrices(int n) {
    var rng = new XorShift32();
    return new Matrices(digits(rng.ints(n * n)), digits(rng.ints(n * n)), n);
  }

  private static double[] digits(long[] values) {
    var out = new double[values.length];
    for (int i = 0; i < values.length; i++) {
      out[i] = values[i] % 10;
    }
    return out;
  }

  static long h31(long[] values) {
    long h = 0;
    for (long v : values) {
      h = (h * 31 + v) & MASK;
    }
    return h;
  }

  static long h31(double[] values) {
    long h = 0;
    for (double v : values) {
      h = (h * 31 + (long) v) & MASK;
    }
    return h;
  }

  static String fnv1a(byte[] data) {
    int h = FNV_OFFSET;
    for (byte b : data) {
      h = (h ^ (b & 0xFF)) * FNV_PRIME;
    }
    return Integer.toUnsignedString(h);
  }

  static String fnv1a(String text) {
    return fnv1a(text.getBytes(StandardCharsets.UTF_8));
  }
}
