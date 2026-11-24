// Race: sieve (goose)
// Extracted from suite/goose/misc.goose — build and run via suite/run.py

import std;
import dictionary;
import regex;
import gen;

fn sieve(n: i64) -> i64 {
    var composite: bool[>..] = [];
    composite.resize(n + 1, false);
    var i = 2;
    while i * i <= n {
        if !composite[i] {
            var j = i * i;
            while j <= n {
                composite[j] = true;
                j += i;
            }
        }
        i++;
    }
    var count = 0;
    for p in 2..n + 1 {
        if !composite[p] { count++; }
    }
    return count;
}
