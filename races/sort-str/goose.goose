// Race: sort-str (goose)
// Extracted from suite/goose/main.goose — build and run via suite/run.py

import std;
import os;
import gen;
import sorts;
import bg;
import misc;
import regex;
import algo;
import puzzle;
import puzzle_text;
import sudoku;
import json_kernel;
import work;
import ways;

struct Opts { size: i64 = 0, repeats: i64 = 1, threads: i64 = 1, data: u8[..1024] = "." }

fn lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

fn random_strings(n: i64) -> u8[..16][>..] {
    var r = xorshift();
    var out: u8[..16][>..] = [];
    for i in n {
        let len = 8 + r.next() % 9;
        let s .= out.push([]);
        for j in len { s.push((97 + r.next() % 26) as! u8); }
    }
    return out;
}

fn sort_str(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let base = random_strings(o.size);
    for rep in o.repeats {
        var a = copy(base);
        let t0 = clock_ns();
        sort(a) { x, y => compare(x, y) < 0 };
        lap(times, t0);
        if rep == o.repeats - 1 {
            var h = FNV_OFFSET;
            for s in a {
                h = fnv1a(s, h);
                h = fnv1a("\n", h);
            }
            format(sum, h);
        }
    }
}

fn format(out: u8[>..]&, m: Matches) { format(out, m.count, ":", m.first, ":", m.last); }
