// Race: bmap-int.go.map (java)
// Extracted from suite/java/src/main/java/kernels/WaysMaps.java — build and run via suite/run.py

package kernels;
import it.unimi.dsi.fastutil.longs.Long2LongOpenHashMap;
import it.unimi.dsi.fastutil.objects.Object2LongOpenHashMap;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;
import org.eclipse.collections.impl.map.mutable.primitive.LongLongHashMap;
import org.eclipse.collections.impl.map.mutable.primitive.ObjectLongHashMap;

import it.unimi.dsi.fastutil.longs.Long2LongOpenHashMap;

import org.eclipse.collections.impl.map.mutable.primitive.LongLongHashMap;

final class WaysMaps {

  static String intHashMap(Misc.HashmapInput in) {
    return Misc.hashmap(in);
  }

  static String intFastutil(Misc.HashmapInput in) {
    var counts = new Long2LongOpenHashMap();
    for (long k : in.inserts()) {
      counts.addTo(k, 1);
    }
    int hits = 0;
    for (long k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

  static String intEclipse(Misc.HashmapInput in) {
    var counts = new LongLongHashMap();
    for (long k : in.inserts()) {
      counts.addToValue(k, 1);
    }
    int hits = 0;
    for (long k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    return counts.size() + ":" + hits;
  }

}
