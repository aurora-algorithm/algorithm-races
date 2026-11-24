// Race: kmp (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

struct Matches { count: i64 = 0, first: i64 = -1, last: i64 = -1 }

fn kmp(t: u8[:], pattern: u8[:]) -> Matches {
    let m = pattern.len;
    var fail: i64[>..] = [];
    fail.resize(m, 0);
    var k = 0;
    for i in 1..m {
        while k > 0 && pattern[i] != pattern[k] { k = fail[k - 1]; }
        if pattern[i] == pattern[k] { k++; }
        fail[i] = k;
    }
    var res = Matches {};
    k = 0;
    for ch, i in t {
        while k > 0 && ch != pattern[k] { k = fail[k - 1]; }
        if ch == pattern[k] { k++; }
        if k == m {
            res.last = i - m + 1;
            if res.first < 0 { res.first = res.last; }
            res.count++;
            k = fail[k - 1];
        }
    }
    return res;
}
