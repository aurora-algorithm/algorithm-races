// Library and loop kernels (kernels/SPEC.md section 5).

import std;
import dictionary;
import regex;
import gen;

let METHODS = ["GET", "POST", "PUT", "DELETE"];
let STATUSES = [200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503];

fn log_text(lines: i64) -> u8[>..] {
    var r = xorshift();
    var out: u8[>..] = [];
    for i in lines {
        let method = METHODS[r.next() % 4];
        let version = 1 + r.next() % 3;
        let user = r.next() % 100000;
        let status = STATUSES[r.next() % 12];
        let ms = r.next() % 1000;
        format(out, method, " /api/v", version, "/users/", user, " ", status, " ", ms, "ms\n");
    }
    return out;
}

fn regex_log(re: regex::Regex&, text: u8[:], out: u8[>..]&) {
    var count = 0;
    var ids = 0;
    regex::each_captures(re, text) { c =>
        let id, ok = parse_int(regex::group(c, 2));
        assert(ok);
        count++;
        ids += id;
    };
    format(out, count, ":", ids);
}

fn hashmap(inserts: i64[:], lookups: i64[:], out: u8[>..]&) {
    var counts = dictionary<i64, i64> {};
    for k in inserts { counts.update(k, 0) { it += 1; }; }
    var hits = 0;
    for k in lookups {
        if counts.contains(k) { hits++; }
    }
    format(out, counts.count, ":", hits);
}

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

fn matrix(r: XorShift32&, n: i64) -> f64[>..] {
    var m: f64[>..] = [];
    for i in n * n { m.push(r.next() % 10); }
    return m;
}

fn matmul(a: f64[:], b: f64[:], c: f64[:], n: i64) {
    for i in n {
        let row = i * n;
        for j in n {
            var s = 0.0;
            for k in n { s += a[row + k] * b[k * n + j]; }
            c[row + j] = s;
        }
    }
}

fn matmul_checksum(c: f64[:], out: u8[>..]&) {
    var total = 0;
    var h: u32 = 0;
    for v in c {
        let x = v as i64;
        total += x;
        h = h31_step(h, x);
    }
    format(out, total, ":", h);
}
