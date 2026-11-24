// Race: edit-distance (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn edit_distance(a: u8[:], b: u8[:]) -> i64 {
    let m = b.len;
    var rows: i64[>..] = [];
    for j in m + 1 { rows.push(j); }
    rows.resize(2 * (m + 1), 0);
    var prev: i64[:] = rows[..m + 1];
    var cur:  i64[:] = rows[m + 1..];
    for i in 1..a.len + 1 {
        cur[0] = i;
        let ca = a[i - 1];
        for j in 1..m + 1 {
            let cost = if ca == b[j - 1] { 0 } else { 1 };
            cur[j] = min(min(prev[j] + 1, cur[j - 1] + 1), prev[j - 1] + cost);
        }
        swap(prev, cur);
    }
    return prev[m];
}
