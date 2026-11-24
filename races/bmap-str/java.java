// Race: bmap-str.go.map (java)
// Extracted from suite/java/src/main/java/kernels/WaysMaps.java — build and run via suite/run.py

package kernels;
import it.unimi.dsi.fastutil.longs.Long2LongOpenHashMap;
import it.unimi.dsi.fastutil.objects.Object2LongOpenHashMap;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;
import org.eclipse.collections.impl.map.mutable.primitive.LongLongHashMap;
import org.eclipse.collections.impl.map.mutable.primitive.ObjectLongHashMap;

import it.unimi.dsi.fastutil.objects.Object2LongOpenHashMap;

import java.util.HashMap;

import org.eclipse.collections.impl.map.mutable.primitive.ObjectLongHashMap;

final class WaysMaps {

  record StrInput(String[] inserts, String[] lookups) {}

  /** Even i looks up a key that is in the map; odd i the key without its last letter. */
  static StrInput strInput(int n) {
    var inserts = Gen.strings(n);
    var lookups = new String[n];
    for (int i = 0; i < n; i++) {
      var k = inserts[i];
      lookups[i] = i % 2 == 0 ? k : k.substring(0, k.length() - 1);
    }
    return new StrInput(inserts, lookups);
  }

  /**
   * Every repeat gets new String objects with the same text, for the inserts and separately for the
   * lookups (made untimed): a String caches its hash after the first hashCode(), and an equal key
   * that is the same object passes equals() by reference. new String(s) would share the cached
   * hash.
   */
  static StrInput freshStrings(StrInput in) {
    return new StrInput(copyAll(in.inserts()), copyAll(in.lookups()));
  }

  private static String[] copyAll(String[] keys) {
    var out = new String[keys.length];
    for (int i = 0; i < keys.length; i++) {
      out[i] = new String(keys[i].toCharArray());
    }
    return out;
  }

  static String strHashMap(StrInput in) {
    var counts = new HashMap<String, Long>();
    for (var k : in.inserts()) {
      counts.merge(k, 1L, Long::sum);
    }
    int hits = 0;
    for (var k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

  static String strFastutil(StrInput in) {
    var counts = new Object2LongOpenHashMap<String>();
    for (var k : in.inserts()) {
      counts.addTo(k, 1);
    }
    int hits = 0;
    for (var k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

  static String strEclipse(StrInput in) {
    var counts = new ObjectLongHashMap<String>();
    for (var k : in.inserts()) {
      counts.addToValue(k, 1);
    }
    int hits = 0;
    for (var k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

}
