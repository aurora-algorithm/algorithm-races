// Race: ordered-map (java)
// Extracted from suite/java/src/main/java/kernels/AlgoTrees.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

import java.util.TreeMap;

final class AlgoTrees {

  static String orderedMap(Misc.HashmapInput in) {
    var counts = new TreeMap<Long, Long>();
    for (long k : in.inserts()) {
      counts.merge(k, 1L, Long::sum);
    }
    int hits = 0;
    for (long k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    var keys = counts.keySet().stream().mapToLong(Long::longValue).toArray();
    return counts.size() + ":" + hits + ":" + Gen.h31(keys);
  }

}
