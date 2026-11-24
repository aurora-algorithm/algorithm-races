// Race: bfs-grid (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn grid(n: i64) -> u8[>..] {
    var r = xorshift();
    var walls: u8[>..] = [];
    for i in n * n { walls.push(if r.next() % 100 < 30 { 1 } else { 0 }); }
    for cell in [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1] { walls[cell] = 0; }
    return walls;
}

// The queue is an array with a head index (Goose has no deque); every cell enters it at most once.
fn bfs_grid(n: i64, walls: u8[:], out: u8[>..]&) {
    var dist: i64[>..] = [];
    dist.resize(n * n, -1);
    var queue: i64[>..] = [];
    dist[0] = 0;
    queue.push(0);
    var head = 0;
    while head < queue.len {
        let cell = queue[head];
        head++;
        let d = dist[cell] + 1;
        let row = cell / n;
        let col = cell % n;
        if row > 0 && walls[cell - n] == 0 && dist[cell - n] < 0 {
            dist[cell - n] = d;
            queue.push(cell - n);
        }
        if row < n - 1 && walls[cell + n] == 0 && dist[cell + n] < 0 {
            dist[cell + n] = d;
            queue.push(cell + n);
        }
        if col > 0 && walls[cell - 1] == 0 && dist[cell - 1] < 0 {
            dist[cell - 1] = d;
            queue.push(cell - 1);
        }
        if col < n - 1 && walls[cell + 1] == 0 && dist[cell + 1] < 0 {
            dist[cell + 1] = d;
            queue.push(cell + 1);
        }
    }
    var reached = 0;
    var total = 0;
    for d in dist {
        if d >= 0 {
            reached++;
            total += d;
        }
    }
    format(out, reached, ":", dist[n * n - 1], ":", total);
}
