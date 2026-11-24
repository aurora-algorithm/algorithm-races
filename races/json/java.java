// Race: json (java)
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

import com.fasterxml.jackson.databind.ObjectMapper;

import com.fasterxml.jackson.databind.PropertyNamingStrategies;

import com.fasterxml.jackson.databind.json.JsonMapper;

import java.nio.charset.StandardCharsets;

import java.util.List;

final class Misc {

  record Order(
      long id, long customerId, String status, long totalCents, int items, List<String> tags) {}

  record Orders(List<Order> orders) {}

  record JsonResult(int orders, long totalCents, long items, String out) {}

  static ObjectMapper mapper() {
    return JsonMapper.builder().propertyNamingStrategy(PropertyNamingStrategies.SNAKE_CASE).build();
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
