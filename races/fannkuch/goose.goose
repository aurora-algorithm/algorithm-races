// Race: fannkuch (goose)
// Extracted from suite/goose/bg.goose — build and run via suite/run.py

import std;
import math;

fn fannkuch(n: i64, out: u8[>..]&) {
    var perm1: i64[>..] = [];
    var count: i64[>..] = [];
    var perm:  i64[>..] = [];
    for i in n {
        perm1.push(i);
        count.push(0);
        perm.push(0);
    }
    var max_flips = 0;
    var checksum = 0;
    var perm_count = 0;
    var r = n;
    loop {
        while r != 1 {
            count[r - 1] = r;
            r--;
        }
        copy_into(perm, perm1);
        var flips = 0;
        var k = perm[0];
        while k != 0 {
            reverse(perm[..k + 1]);
            flips++;
            k = perm[0];
        }
        max_flips = max(max_flips, flips);
        checksum += if perm_count % 2 == 0 { flips } else { -flips };
        loop {
            if r == n {
                format(out, checksum, ":", max_flips);
                return;
            }
            let perm0 = perm1[0];
            for i in r { perm1[i] = perm1[i + 1]; }
            perm1[r] = perm0;
            count[r]--;
            if count[r] > 0 { break; }
            r++;
        }
        perm_count++;
    }
}
