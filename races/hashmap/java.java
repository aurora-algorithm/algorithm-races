// Race: hashmap (java)
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

import java.util.HashMap;

final class Misc {

  record HashmapInput(long[] inserts, long[] lookups) {}

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

}
