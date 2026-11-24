// Race: palindrome (goose)
// Extracted from suite/goose/puzzle_text.goose — build and run via suite/run.py

import std;
import gen;

fn palindrome(text: u8[:], out: u8[>..]&) {
    var t: u8[>..] = [];
    t.push('^');
    t.push('#');
    for c in text {
        t.push(c);
        t.push('#');
    }
    t.push('$');
    var p: i64[>..] = [];
    p.resize(t.len, 0);
    var center = 0;
    var right = 0;
    for i in 1..t.len - 1 {
        if i < right { p[i] = min(right - i, p[2 * center - i]); }
        while t[i + 1 + p[i]] == t[i - 1 - p[i]] { p[i]++; }
        if i + p[i] > right {
            center = i;
            right = i + p[i];
        }
    }
    var longest = 0;
    var best = 0;
    var count = 0;
    for radius, i in p {
        count += (radius + 1) / 2;
        if radius > longest {
            longest = radius;
            best = i;
        }
    }
    format(out, text.len, ":", longest, ":", (best - 1 - longest) / 2, ":", count);
}
