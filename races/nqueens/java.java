// Race: nqueens (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  static long nqueens(int n) {
    return new Board(n).place(0);
  }

  private static final class Board {
    private final int n;
    private final boolean[] cols;
    private final boolean[] up;
    private final boolean[] down;

    Board(int n) {
      this.n = n;
      cols = new boolean[n];
      up = new boolean[2 * n - 1];
      down = new boolean[2 * n - 1];
    }

    long place(int row) {
      if (row == n) {
        return 1;
      }
      long count = 0;
      for (int col = 0; col < n; col++) {
        int u = row + col;
        int d = row + n - 1 - col;
        if (!cols[col] && !up[u] && !down[d]) {
          cols[col] = up[u] = down[d] = true;
          count += place(row + 1);
          cols[col] = up[u] = down[d] = false;
        }
      }
      return count;
    }
  }

}
