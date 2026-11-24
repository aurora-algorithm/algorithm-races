// Race: cycle (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

let CYCLE_MOD = 67108859;

let CYCLE_C = 12345;

fn step(x: i64) -> i64 { (x * x + CYCLE_C) % CYCLE_MOD }

fn cycle(starts: i64[:], out: u8[>..]&) {
    var total_mu = 0;
    var total_lam = 0;
    var longest = 0;
    for x0 in starts {
        var tortoise = step(x0);
        var hare = step(tortoise);
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        var mu = 0;
        tortoise = x0;
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(hare);
            mu++;
        }
        var lam = 1;
        hare = step(tortoise);
        while tortoise != hare {
            hare = step(hare);
            lam++;
        }
        total_mu += mu;
        total_lam += lam;
        longest = max(longest, mu + lam);
    }
    format(out, starts.len, ":", total_mu, ":", total_lam, ":", longest);
}
