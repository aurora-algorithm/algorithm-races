// Race: life (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

let LIFE_SIDE = 512;

// Runs the generations in the caller's two-grid buffer `cells` (empty on entry) and returns where the last one
// starts, so the alive count and fnv1a run after the timer.
fn life(grid: u8[:], gens: i64, cells: u8[>..]&) -> i64 {
    let w = LIFE_SIDE;
    cells.append(grid);
    cells.resize(2 * grid.len, 0);
    var cur: u8[:] = cells[..grid.len];
    var nxt: u8[:] = cells[grid.len..];
    for g in gens {
        for y in w {
            let up = (y + w - 1) % w * w;
            let row = y * w;
            let down = (y + 1) % w * w;
            for x in w {
                let left = (x + w - 1) % w;
                let right = (x + 1) % w;
                let n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] + cur[row + right]
                    + cur[down + left] + cur[down + x] + cur[down + right];
                nxt[row + x] = if n == 3 || (n == 2 && cur[row + x] != 0) { 1 } else { 0 };
            }
        }
        swap(cur, nxt);
    }
    return if gens % 2 == 0 { 0 } else { grid.len };
}
