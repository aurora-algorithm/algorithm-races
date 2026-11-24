// Race: knapsack (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  private static final int KNAPSACK_ITEMS = 1000;

  record Knapsack(int capacity, int[] weights, long[] values) {}

  static Knapsack items(int capacity) {
    var rng = new Gen.XorShift32();
    var weights = new int[KNAPSACK_ITEMS];
    var values = new long[KNAPSACK_ITEMS];
    for (int i = 0; i < KNAPSACK_ITEMS; i++) {
      weights[i] = 1 + (int) (rng.next() % 1000);
      values[i] = 1 + rng.next() % 1000;
    }
    return new Knapsack(capacity, weights, values);
  }

  static String knapsack(Knapsack k) {
    int capacity = k.capacity();
    var best = new long[capacity + 1];
    for (int i = 0; i < k.weights().length; i++) {
      int weight = k.weights()[i];
      long value = k.values()[i];
      for (int c = capacity; c >= weight; c--) {
        if (best[c - weight] + value > best[c]) {
          best[c] = best[c - weight] + value;
        }
      }
    }
    return best[capacity] + ":" + Gen.h31(best);
  }

}
