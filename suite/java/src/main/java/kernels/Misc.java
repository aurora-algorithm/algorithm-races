package kernels;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.databind.json.JsonMapper;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.List;
import java.util.regex.Pattern;

/** Library and loop kernels (kernels/SPEC.md section 5). */
final class Misc {
  private static final String LOG_PATTERN = "(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])";

  private Misc() {}

  record LogInput(Pattern pattern, String text) {}

  /** The pattern is compiled here, before the timed region (SPEC section 5). */
  static LogInput logInput(String text) {
    return new LogInput(Pattern.compile(LOG_PATTERN), text);
  }

  static String regexLog(LogInput in) {
    int count = 0;
    long ids = 0;
    var m = in.pattern().matcher(in.text());
    while (m.find()) {
      count++;
      ids += Long.parseLong(m.group(2));
    }
    return count + ":" + ids;
  }

  record HashmapInput(long[] inserts, long[] lookups) {}

  static HashmapInput hashmapInput(int n) {
    var inserts = Gen.ints(n);
    var lookups = new long[n];
    for (int i = 0; i < n; i++) {
      lookups[i] = inserts[i] ^ (i & 1);
    }
    return new HashmapInput(inserts, lookups);
  }

  static String hashmap(HashmapInput input) {
    var counts = new HashMap<Long, Long>();
    for (long k : input.inserts()) {
      counts.merge(k, 1L, Long::sum);
    }
    int hits = 0;
    for (long k : input.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

  static String sha256(byte[] buffer) {
    try {
      var digest = MessageDigest.getInstance("SHA-256").digest(buffer);
      return HexFormat.of().formatHex(digest);
    } catch (NoSuchAlgorithmException e) {
      throw new IllegalStateException("every Java runtime ships SHA-256", e);
    }
  }

  static String sieve(int n) {
    var composite = new boolean[n + 1];
    for (int i = 2; i * i <= n; i++) {
      if (!composite[i]) {
        for (int j = i * i; j <= n; j += i) {
          composite[j] = true;
        }
      }
    }
    int count = 0;
    for (int i = 2; i <= n; i++) {
      if (!composite[i]) {
        count++;
      }
    }
    return Integer.toString(count);
  }

  static double[] matmul(Gen.Matrices m) {
    int n = m.n();
    var a = m.a();
    var b = m.b();
    var c = new double[n * n];
    for (int i = 0; i < n; i++) {
      int row = i * n;
      for (int j = 0; j < n; j++) {
        double s = 0;
        for (int k = 0; k < n; k++) {
          s += a[row + k] * b[k * n + j];
        }
        c[row + j] = s;
      }
    }
    return c;
  }

  static String matmulChecksum(double[] c) {
    long total = 0;
    for (double v : c) {
      total += (long) v;
    }
    return total + ":" + Gen.h31(c);
  }

  record Order(
      long id, long customerId, String status, long totalCents, int items, List<String> tags) {}

  record Orders(List<Order> orders) {}

  record JsonInput(ObjectMapper mapper, String text) {}

  record JsonResult(int orders, long totalCents, long items, String out) {}

  static ObjectMapper mapper() {
    return JsonMapper.builder().propertyNamingStrategy(PropertyNamingStrategies.SNAKE_CASE).build();
  }

  static JsonResult json(JsonInput input) {
    try {
      var doc = input.mapper().readValue(input.text(), Orders.class);
      long total = 0;
      long items = 0;
      for (var order : doc.orders()) {
        total += order.totalCents();
        items += order.items();
      }
      var out = input.mapper().writeValueAsString(doc);
      return new JsonResult(doc.orders().size(), total, items, out);
    } catch (JsonProcessingException e) {
      throw new IllegalArgumentException("orders file is not valid JSON", e);
    }
  }

  static String jsonChecksum(JsonResult r) {
    var bytes = r.out().getBytes(StandardCharsets.UTF_8);
    return r.orders()
        + ":"
        + r.totalCents()
        + ":"
        + r.items()
        + ":"
        + bytes.length
        + ":"
        + Gen.fnv1a(bytes);
  }
}
