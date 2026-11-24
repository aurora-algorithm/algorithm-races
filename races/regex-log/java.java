// Race: regex-log (java)
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

import java.util.regex.Pattern;

final class Misc {

  private static final String LOG_PATTERN = "(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])";

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

}
