// Race: rain-water (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

import java.util.PriorityQueue;

final class Puzzle {

  static int[] terrain(int side) {
    var rng = new Gen.XorShift32();
    var heights = new int[side * side];
    for (int i = 0; i < heights.length; i++) {
      heights[i] = (int) (rng.next() % 1000);
    }
    return heights;
  }

  static String rainWater(int[] heights) {
    int n = heights.length;
    int side = (int) Math.sqrt(n);
    var seen = new boolean[n];
    var heap = new PriorityQueue<Long>();
    for (int idx = 0; idx < n; idx++) {
      int r = idx / side;
      int c = idx % side;
      if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
        seen[idx] = true;
        heap.add((long) heights[idx] * n + idx);
      }
    }
    long water = 0;
    long wet = 0;
    int[] dr = {-1, 1, 0, 0};
    int[] dc = {0, 0, -1, 1};
    while (!heap.isEmpty()) {
      long key = heap.poll();
      long level = key / n;
      int idx = (int) (key % n);
      int r = idx / side;
      int c = idx % side;
      for (int d = 0; d < 4; d++) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        if (nr >= 0 && nr < side && nc >= 0 && nc < side) {
          int nidx = nr * side + nc;
          if (!seen[nidx]) {
            seen[nidx] = true;
            long h = heights[nidx];
            if (h < level) {
              water += level - h;
              wet++;
              h = level;
            }
            heap.add(h * n + nidx);
          }
        }
      }
    }
    return side + ":" + water + ":" + wet;
  }

}
