// The builtins group (kernels/builtins-NOTES.md): the standard library's ways of doing one job, one way per
// kernel, named JOB.goose.WAY. Every way of a job gets the same input and prints the same checksum.

import std;
import dictionary;
import gen;

fn way_lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

fn way_strings(n: i64) -> u8[..16][>..] {
    var r = xorshift();
    var out: u8[..16][>..] = [];
    for i in n {
        let len = 8 + r.next() % 9;
        let s .= out.push([]);
        for j in len { s.push((97 + r.next() % 26) as! u8); }
    }
    return out;
}

// ints(N), or ints(N) sorted ascending (order 1) or descending (order -1); made before timing.
fn way_ints(n: i64, order: i64) -> i64[>..] {
    var a = ints(n);
    if order > 0 { sort(a); }
    if order < 0 { sort(a) { x, y => x > y }; }
    return a;
}

fn way_strings_ordered(n: i64, order: i64) -> u8[..16][>..] {
    var a = way_strings(n);
    if order > 0 { sort(a) { x, y => compare(x, y) < 0 }; }
    if order < 0 { sort(a) { x, y => compare(x, y) > 0 }; }
    return a;
}

fn way_sort_ints<F>(n: i64, order: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let base = way_ints(n, order);
    for rep in repeats {
        var a = copy(base);
        let t0 = clock_ns();
        F(a);
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, h31(a)); }
    }
}

fn way_sort_strings<F>(n: i64, order: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let base = way_strings_ordered(n, order);
    for rep in repeats {
        var a = copy(base);
        let t0 = clock_ns();
        F(a);
        way_lap(times, t0);
        if rep == repeats - 1 {
            var h = FNV_OFFSET;
            for s in a {
                h = fnv1a(s, h);
                h = fnv1a("\n", h);
            }
            format(sum, h);
        }
    }
}

fn way_map_int(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let inserts = ints(n);
    var lookups: i64[>..] = [];
    for k, i in inserts { lookups.push(k ^ (i & 1)); }
    for rep in repeats {
        let t0 = clock_ns();
        var counts = dictionary<i64, i64> {};
        for k in inserts { counts.update(k, 0) { it += 1; }; }
        var hits = 0;
        for k in lookups {
            if counts.contains(k) { hits++; }
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, counts.count, ":", hits); }
    }
}

// Keys are slices of the input strings (no copies). Each repeat gets its own copies of the inserts and, apart from
// them, of the lookups (odd ones without the key's last letter), made before the timer, as in the other languages.
fn way_map_str(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let inserts = way_strings(n);
    var lookups: u8[..16][>..] = [];
    for i in n {
        let k = inserts[i][..];
        let s .= lookups.push([]);
        s.append(if i % 2 == 0 { k } else { k[..k.len - 1] });
    }
    for rep in repeats {
        let ins = copy(inserts);
        let look = copy(lookups);
        let t0 = clock_ns();
        var counts = dictionary<u8[:], i64> {};
        for i in n { counts.update(ins[i][..], 0) { it += 1; }; }
        var hits = 0;
        for i in n {
            if counts.contains(look[i][..]) { hits++; }
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, counts.count, ":", hits); }
    }
}

fn way_join(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    var parts: u8[..10][>..] = [];
    for v in ints(n) {
        let s .= parts.push([]);
        var digits: u8[>..] = [];
        format(digits, v);
        s.append(digits);
    }
    for rep in repeats {
        let t0 = clock_ns();
        var out: u8[>..] = [];
        for p, i in parts {
            if i > 0 { out.push(','); }
            out.append(p);
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, out.len, ":", fnv1a(out)); }
    }
}

// -1, 0 or 1 for the -reversed, random and -sorted inputs of a sort job, or -2 when `job` is no sort job.
fn way_order(job: u8[:], base: u8[:]) -> i64 {
    if job == base { return 0; }
    if job == str(base, "-sorted") { return 1; }
    if job == str(base, "-reversed") { return -1; }
    return -2;
}

fn ways_dispatch(k: u8[:], size: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) -> bool {
    var dot = -1;
    for c, i in k {
        if c == '.' && dot < 0 { dot = i; }
    }
    if dot < 0 { return false; }
    let job = k[..dot];
    let way = k[dot + 1..];
    let oi = way_order(job, "bsort-int");
    let os = way_order(job, "bsort-str");
    if oi > -2 && way == "goose.sort" {
        way_sort_ints(size, oi, repeats, times, sum) { a => sort(a); };
    } else if oi > -2 && way == "goose.stable_sort" {
        way_sort_ints(size, oi, repeats, times, sum) { a => stable_sort(a); };
    } else if os > -2 && way == "goose.sort" {
        way_sort_strings(size, os, repeats, times, sum) { a => sort(a) { x, y => compare(x, y) < 0 }; };
    } else if os > -2 && way == "goose.stable_sort" {
        way_sort_strings(size, os, repeats, times, sum) { a => stable_sort(a) { x, y => compare(x, y) < 0 }; };
    } else if k == "bmap-int.goose.dictionary" {
        way_map_int(size, repeats, times, sum);
    } else if k == "bmap-str.goose.dictionary" {
        way_map_str(size, repeats, times, sum);
    } else if k == "bjoin.goose.append" {
        way_join(size, repeats, times, sum);
    } else {
        return false;
    }
    return true;
}
