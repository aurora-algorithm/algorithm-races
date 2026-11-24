// Race: sudoku (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

import java.util.Arrays;

final class Puzzle {

  static void shuffle(Gen.XorShift32 rng, int[] items) {
    for (int i = items.length - 1; i > 0; i--) {
      int j = (int) (rng.next() % (i + 1));
      int t = items[i];
      items[i] = items[j];
      items[j] = t;
    }
  }

  private static final int SUDOKU_CLUES = 26;

  private static final int DLX_COLUMNS = 324;

  static int[][] sudokuPuzzles(int n) {
    var rng = new Gen.XorShift32();
    var puzzles = new int[n][];
    for (int p = 0; p < n; p++) {
      int[] digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      shuffle(rng, digits);
      var order = new int[2][9];
      for (int axis = 0; axis < 2; axis++) {
        int[] bands = {0, 1, 2};
        shuffle(rng, bands);
        int line = 0;
        for (int band : bands) {
          int[] inner = {0, 1, 2};
          shuffle(rng, inner);
          for (int i : inner) {
            order[axis][line++] = band * 3 + i;
          }
        }
      }
      int[] rows = order[0];
      int[] cols = order[1];
      var cells = new int[81];
      for (int i = 0; i < 81; i++) {
        cells[i] = i;
      }
      shuffle(rng, cells);
      var puzzle = new int[81];
      for (int k = 0; k < SUDOKU_CLUES; k++) {
        int cell = cells[k];
        int r = rows[cell / 9];
        int c = cols[cell % 9];
        puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
      }
      puzzles[p] = puzzle;
    }
    return puzzles;
  }

  /** The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the headers, then rows. */
  static final class Dlx {
    final int[] left;
    final int[] right;
    final int[] up;
    final int[] down;
    final int[] col;
    final int[] row;
    final int[] count = new int[DLX_COLUMNS + 1];
    final int[] chosen = new int[81];
    int[] first = new int[0];
    long solutions;
    long tries;

    Dlx() {
      int size = 1 + DLX_COLUMNS + 729 * 4;
      left = new int[size];
      right = new int[size];
      up = new int[size];
      down = new int[size];
      col = new int[size];
      row = new int[size];
      for (int i = 0; i < size; i++) {
        up[i] = i;
        down[i] = i;
        col[i] = i;
      }
      for (int i = 0; i <= DLX_COLUMNS; i++) {
        left[i] = i - 1;
        right[i] = i + 1;
      }
      left[0] = DLX_COLUMNS;
      right[DLX_COLUMNS] = 0;
      int node = DLX_COLUMNS + 1;
      for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
          int box = (r / 3) * 3 + c / 3;
          for (int d = 0; d < 9; d++) {
            int first = node;
            int[] columns = {1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d, 244 + box * 9 + d};
            for (int column : columns) {
              col[node] = column;
              row[node] = (r * 9 + c) * 9 + d;
              up[node] = up[column];
              down[node] = column;
              down[up[column]] = node;
              up[column] = node;
              count[column]++;
              left[node] = node - 1;
              right[node] = node + 1;
              node++;
            }
            left[first] = node - 1;
            right[node - 1] = first;
          }
        }
      }
    }

    void cover(int c) {
      right[left[c]] = right[c];
      left[right[c]] = left[c];
      for (int i = down[c]; i != c; i = down[i]) {
        for (int j = right[i]; j != i; j = right[j]) {
          down[up[j]] = down[j];
          up[down[j]] = up[j];
          count[col[j]]--;
        }
      }
    }

    void uncover(int c) {
      for (int i = up[c]; i != c; i = up[i]) {
        for (int j = left[i]; j != i; j = left[j]) {
          count[col[j]]++;
          down[up[j]] = j;
          up[down[j]] = j;
        }
      }
      right[left[c]] = c;
      left[right[c]] = c;
    }

    void chooseRow(int node) {
      for (int j = right[node]; j != node; j = right[j]) {
        cover(col[j]);
      }
    }

    void unchooseRow(int node) {
      for (int j = left[node]; j != node; j = left[j]) {
        uncover(col[j]);
      }
    }

    void search(int depth) {
      if (right[0] == 0) {
        solutions++;
        if (solutions == 1) {
          first = Arrays.copyOf(chosen, depth);
        }
        return;
      }
      int best = right[0];
      for (int c = right[best]; c != 0; c = right[c]) {
        if (count[c] < count[best]) {
          best = c;
        }
      }
      cover(best);
      for (int r = down[best]; r != best; r = down[r]) {
        tries++;
        chosen[depth] = row[r];
        chooseRow(r);
        search(depth + 1);
        unchooseRow(r);
      }
      uncover(best);
    }
  }

  static String sudoku(int[][] puzzles) {
    long solutions = 0;
    long tries = 0;
    long h = 0;
    for (int[] puzzle : puzzles) {
      var dlx = new Dlx();
      int depth = 0;
      for (int cell = 0; cell < 81; cell++) {
        int digit = puzzle[cell];
        if (digit != 0) {
          int node = DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4;
          dlx.cover(dlx.col[node]);
          dlx.chooseRow(node);
          dlx.chosen[depth++] = cell * 9 + digit - 1;
        }
      }
      dlx.search(depth);
      solutions += dlx.solutions;
      tries += dlx.tries;
      var chain = new long[82];
      chain[0] = h;
      for (int row : dlx.first) {
        chain[1 + row / 9] = row % 9 + 1;
      }
      h = Gen.h31(chain);
    }
    return puzzles.length + ":" + solutions + ":" + tries + ":" + h;
  }

}
