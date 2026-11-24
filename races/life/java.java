// Race: life (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

final class Puzzle {

  private static final int LIFE_SIDE = 512;

  record LifeInput(byte[] grid, int gens) {}

  record LifeResult(int gens, byte[] grid) {}

  static LifeInput lifeInput(int gens) {
    var rng = new Gen.XorShift32();
    var grid = new byte[LIFE_SIDE * LIFE_SIDE];
    for (int i = 0; i < grid.length; i++) {
      grid[i] = (byte) (rng.next() % 4 == 0 ? 1 : 0);
    }
    return new LifeInput(grid, gens);
  }

  static LifeResult life(LifeInput in) {
    int w = LIFE_SIDE;
    var cur = in.grid().clone();
    var nxt = new byte[cur.length];
    for (int g = 0; g < in.gens(); g++) {
      for (int y = 0; y < w; y++) {
        int up = (y + w - 1) % w * w;
        int row = y * w;
        int down = (y + 1) % w * w;
        for (int x = 0; x < w; x++) {
          int left = (x + w - 1) % w;
          int right = (x + 1) % w;
          int n =
              cur[up + left]
                  + cur[up + x]
                  + cur[up + right]
                  + cur[row + left]
                  + cur[row + right]
                  + cur[down + left]
                  + cur[down + x]
                  + cur[down + right];
          nxt[row + x] = (byte) (n == 3 || (n == 2 && cur[row + x] != 0) ? 1 : 0);
        }
      }
      var t = cur;
      cur = nxt;
      nxt = t;
    }
    return new LifeResult(in.gens(), cur);
  }

  static String lifeChecksum(LifeResult r) {
    long alive = 0;
    for (byte b : r.grid()) {
      alive += b;
    }
    return r.gens() + ":" + alive + ":" + Gen.fnv1a(r.grid());
  }

}
