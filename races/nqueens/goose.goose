// Race: nqueens (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn nqueens(n: i64) -> i64 {
    var cols: bool[>..] = [];
    var up:   bool[>..] = [];
    var down: bool[>..] = [];
    cols.resize(n, false);
    up.resize(2 * n - 1, false);
    down.resize(2 * n - 1, false);
    recursive fn place(row: i64) -> i64 {
        if row == n { return 1; }
        var count = 0;
        for col in n {
            if !cols[col] && !up[row + col] && !down[row - col + n - 1] {
                cols[col] = true;
                up[row + col] = true;
                down[row - col + n - 1] = true;
                count += place(row + 1);
                cols[col] = false;
                up[row + col] = false;
                down[row - col + n - 1] = false;
            }
        }
        return count;
    }
    return place(0);
}
