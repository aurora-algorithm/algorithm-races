// Race: matmul (java)
// Extracted from suite/java/src/main/java/kernels/Misc.java — build and run via suite/run.py

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

final class Misc {

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

}
