// Race: sieve (java)
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

}
