// Race: sha256 (java)
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

import java.security.MessageDigest;

import java.security.NoSuchAlgorithmException;

import java.util.HexFormat;

final class Misc {

  static String sha256(byte[] buffer) {
    try {
      var digest = MessageDigest.getInstance("SHA-256").digest(buffer);
      return HexFormat.of().formatHex(digest);
    } catch (NoSuchAlgorithmException e) {
      throw new IllegalStateException("every Java runtime ships SHA-256", e);
    }
  }

}
