// Kernel runner for Goose (kernels/SPEC.md section 1).
//
//   kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
//
// Prints one JSON line: kernel, size, threads, checksum, times_ms (one per repeat, kernel only), runtime.

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

// A sort of ints(N): a fresh copy per repeat, made outside the timed region; checksum h31 of the last result.
fn sort_kernel<F>(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let base = ints(o.size);
    for rep in o.repeats {
        var a = copy(base);
        let t0 = clock_ns();
        F(a);
        lap(times, t0);
        if rep == o.repeats - 1 { format(sum, h31(a)); }
    }
}

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

// A kernel whose only input is the size and whose result is text: F(n, out) writes it inside the timed region.
// `hash` puts fnv1a of the text in the checksum instead of the text itself.
fn text_kernel<F>(o: Opts, times: f64[>..]&, sum: u8[>..]&, hash: bool) {
    for rep in o.repeats {
        var out: u8[>..] = [];
        let t0 = clock_ns();
        F(o.size, out);
        lap(times, t0);
        if rep == o.repeats - 1 {
            if hash { format(sum, fnv1a(out)); } else { sum.append(out); }
        }
    }
}

fn hashmap_kernel(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let inserts = ints(o.size);
    var lookups: i64[>..] = [];
    for k, i in inserts { lookups.push(k ^ (i & 1)); }
    for rep in o.repeats {
        var out: u8[>..] = [];
        let t0 = clock_ns();
        hashmap(inserts, lookups, out);
        lap(times, t0);
        if rep == o.repeats - 1 { sum.append(out); }
    }
}

fn regex_kernel(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let text = log_text(o.size);
    let re = regex::compile("(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])");
    for rep in o.repeats {
        var out: u8[>..] = [];
        let t0 = clock_ns();
        regex_log(re, text, out);
        lap(times, t0);
        if rep == o.repeats - 1 { sum.append(out); }
    }
}

fn matmul_kernel(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let n = o.size;
    var r = xorshift();
    let a = matrix(r, n);
    let b = matrix(r, n);
    var c: f64[>..] = [];
    c.resize(n * n, 0.0);
    for rep in o.repeats {
        let t0 = clock_ns();
        matmul(a, b, c, n);
        lap(times, t0);
    }
    matmul_checksum(c, sum);
}

// The common case: the input is built by the caller before this, and F(out) runs the kernel and writes its
// checksum text inside the timed region (the checksum is a small part of the work, kernels/SPEC.md section 12).
fn timed<F>(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    for rep in o.repeats {
        var out: u8[>..] = [];
        let t0 = clock_ns();
        F(out);
        lap(times, t0);
        if rep == o.repeats - 1 { sum.append(out); }
    }
}

fn format(out: u8[>..]&, m: Matches) { format(out, m.count, ":", m.first, ":", m.last); }

fn kruskal_kernel(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    let base = edge_list(o.size);
    for rep in o.repeats {
        var edges = copy(base);
        var out: u8[>..] = [];
        let t0 = clock_ns();
        kruskal(o.size, edges, out);
        lap(times, t0);
        if rep == o.repeats - 1 { sum.append(out); }
    }
}

fn json_kernel(o: Opts, times: f64[>..]&, sum: u8[>..]&) {
    var text: u8[>..] = [];
    let path = str(o.data, "/orders-", o.size, ".json");
    if !read_file(path, text) { abort(str("cannot read ", path)); }
    for rep in o.repeats {
        var out: u8[>..] = [];
        let t0 = clock_ns();
        let t = json_roundtrip(text, out);
        lap(times, t0);
        if rep == o.repeats - 1 {
            format(sum, t.orders, ":", t.total_cents, ":", t.items, ":", out.len, ":", fnv1a(out));
        }
    }
}

fn algo_dispatch(k: u8[:], o: Opts, times: f64[>..]&, sum: u8[>..]&) -> bool {
    let n = o.size;
    if k == "bfs-grid" {
        let walls = grid(n);
        timed(o, times, sum) { out => bfs_grid(n, walls, out); };
    } else if k == "dijkstra" {
        var to:     i64[>..] = [];
        var weight: i64[>..] = [];
        graph(n, to, weight);
        timed(o, times, sum) { out => dijkstra(n, to, weight, out); };
    } else if k == "kruskal" {
        kruskal_kernel(o, times, sum);
    } else if k == "bst-arena" {
        let inserts = ints(n);
        var lookups: i64[>..] = [];
        for v, i in inserts { lookups.push(v ^ (i & 1)); }
        timed(o, times, sum) { out => bst(inserts, lookups, out); };
    } else if k == "trie-arena" {
        var inserted: u8[..10][>..] = [];
        var prefixes: u8[..4][>..]  = [];
        words(n, inserted, prefixes);
        timed(o, times, sum) { out => trie(inserted, prefixes, out); };
    } else if k == "lru" {
        let capacity = max(1, n / 100);
        var r = xorshift();
        var keys: i64[>..] = [];
        for i in n { keys.push(r.next() % (2 * capacity)); }
        timed(o, times, sum) { out => lru(capacity, keys, out); };
    } else if k == "edit-distance" {
        var r = xorshift();
        let a = dna(r, n);
        let b = dna(r, n);
        timed(o, times, sum) { out => format(out, edit_distance(a, b)); };
    } else if k == "knapsack" {
        let things = items();
        timed(o, times, sum) { out => knapsack(n, things, out); };
    } else if k == "lis" {
        let values = ints(n);
        timed(o, times, sum) { out => lis(values, out); };
    } else if k == "kmp" || k == "str-find" {
        var r = xorshift();
        let t = dna(r, n);
        let pattern = t[n / 2..n / 2 + PATTERN_LEN];
        let hand = k == "kmp";
        timed(o, times, sum) { out => format(out, if hand { kmp(t, pattern) } else { str_find(t, pattern) }); };
    } else if k == "fib" {
        timed(o, times, sum) { out => format(out, fib(n)); };
    } else if k == "nqueens" {
        timed(o, times, sum) { out => format(out, nqueens(n)); };
    } else if k == "binary-search" {
        var a:       i64[>..] = [];
        var queries: i64[>..] = [];
        sorted_array(n, a, queries);
        timed(o, times, sum) { out => binary_search(a, queries, out); };
    } else {
        return false;
    }
    return true;
}

fn puzzle_dispatch(k: u8[:], o: Opts, times: f64[>..]&, sum: u8[>..]&) -> bool {
    let n = o.size;
    if k == "sudoku" {
        let puzzles = sudoku_puzzles(n);
        var d = Dlx { .. };
        timed(o, times, sum) { out => sudoku(puzzles, d, out); };
    } else if k == "bloom" {
        let values = ints(2 * n);
        timed(o, times, sum) { out => bloom(values[..n], values[n..], out); };
    } else if k == "hyperloglog" {
        timed(o, times, sum) { out => hyperloglog(n, out); };
    } else if k == "skiplist" {
        let keys = ints(n);
        var lookups: i64[>..] = [];
        for v, i in keys { lookups.push(v ^ (i & 1)); }
        for rep in o.repeats {
            var key:  i64[>..] = [];
            var base: i64[>..] = [];
            var next: i64[>..] = [];
            var out:  u8[>..]  = [];
            let t0 = clock_ns();
            skiplist(keys, lookups, key, base, next, out);
            lap(times, t0);
            if rep == o.repeats - 1 { format(sum, out, skiplist_walk(key, base, next)); }
        }
    } else if k == "cycle" {
        var r = xorshift();
        var starts: i64[>..] = [];
        for i in n { starts.push(r.next() % CYCLE_MOD); }
        timed(o, times, sum) { out => cycle(starts, out); };
    } else if k == "palindrome" {
        let text = binary_text(n);
        timed(o, times, sum) { out => palindrome(text, out); };
    } else if k == "bwt" {
        let text = dna_text(n);
        for rep in o.repeats {
            var result: u8[>..] = [];
            var out:    u8[>..] = [];
            let t0 = clock_ns();
            bwt(text, result, out);
            lap(times, t0);
            if rep == o.repeats - 1 { format(sum, out, fnv1a(result)); }
        }
    } else if k == "aho-corasick" {
        var patterns: u8[..12][>..] = [];
        var text:     u8[>..]       = [];
        aho_input(n, patterns, text);
        timed(o, times, sum) { out => aho_corasick(patterns, text, out); };
    } else if k == "life" {
        let g = life_grid();
        for rep in o.repeats {
            var cells: u8[>..] = [];
            let t0 = clock_ns();
            let start = life(g, n, cells);
            lap(times, t0);
            if rep == o.repeats - 1 {
                let last = cells[start..start + g.len];
                var alive = 0;
                for c in last { alive += c; }
                format(sum, n, ":", alive, ":", fnv1a(last));
            }
        }
    } else if k == "huffman" {
        let text = skewed_text(n);
        for rep in o.repeats {
            var encoded: u8[>..] = [];
            var out:     u8[>..] = [];
            let t0 = clock_ns();
            let ok = huffman(text, encoded, out);
            lap(times, t0);
            if rep == o.repeats - 1 { format(sum, out, fnv1a(encoded), ":", ok); }
        }
    } else if k == "window-max" {
        let values = ints(n);
        timed(o, times, sum) { out => window_max(values, out); };
    } else if k == "rain-water" {
        var r = xorshift();
        let heights = ints_mod(r, n * n, 1000);
        timed(o, times, sum) { out => rain_water(heights, out); };
    } else if k == "collatz" {
        timed(o, times, sum) { out => collatz(n, out); };
    } else {
        return false;
    }
    return true;
}

// Runs the named kernel; false when this program has no such kernel.
fn dispatch(k: u8[:], o: Opts, times: f64[>..]&, sum: u8[>..]&) -> bool {
    if k == "sort-int" {
        sort_kernel(o, times, sum) { a => sort(a); };
    } else if k == "sort-str" {
        sort_str(o, times, sum);
    } else if k == "quicksort" {
        sort_kernel(o, times, sum) { a => run_quicksort(a); };
    } else if k == "mergesort" {
        sort_kernel(o, times, sum) { a => run_mergesort(a); };
    } else if k == "heapsort" {
        sort_kernel(o, times, sum) { a => heapsort(a); };
    } else if k == "radixsort" {
        sort_kernel(o, times, sum) { a => radixsort(a); };
    } else if k == "binary-trees-arena" {
        text_kernel(o, times, sum, true) { n, out => binary_trees(n, out); };
    } else if k == "nbody" {
        text_kernel(o, times, sum, false) { n, out => nbody(n, out); };
    } else if k == "mandelbrot" {
        text_kernel(o, times, sum, true) { n, out => out.append(mandelbrot(n)); };
    } else if k == "fannkuch" {
        text_kernel(o, times, sum, false) { n, out => fannkuch(n, out); };
    } else if k == "spectral-norm" {
        text_kernel(o, times, sum, false) { n, out => spectral_norm(n, out); };
    } else if k == "regex-log" {
        regex_kernel(o, times, sum);
    } else if k == "hashmap" {
        hashmap_kernel(o, times, sum);
    } else if k == "sieve" {
        text_kernel(o, times, sum, false) { n, out => format(out, sieve(n)); };
    } else if k == "matmul" {
        matmul_kernel(o, times, sum);
    } else if k == "json" {
        json_kernel(o, times, sum);
    } else if k == "csv" {
        let path = str(o.data, "/orders-", o.size, ".csv");
        timed(o, times, sum) { out => csv_orders(path, out); };
    } else if k == "wordcount" {
        let path = str(o.data, "/text-", o.size, ".txt");
        timed(o, times, sum) { out => wordcount(path, out); };
    } else {
        return algo_dispatch(k, o, times, sum) || puzzle_dispatch(k, o, times, sum)
            || ways_dispatch(k, o.size, o.repeats, times, sum);
    }
    return true;
}

fn parse_opts(argv: u8[:][:], o: Opts&) -> bool {
    var i = 2;
    while i + 1 < argv.len {
        let v, ok = parse_int(argv[i + 1]);
        let flag = argv[i];
        if flag == "--size" && ok {
            o.size = v;
        } else if flag == "--repeats" && ok {
            o.repeats = v;
        } else if flag == "--threads" && ok {
            o.threads = v;
        } else if flag == "--data" {
            o.data.clear();
            o.data.append(argv[i + 1]);
        } else {
            return false;
        }
        i += 2;
    }
    return i == argv.len && o.size > 0 && o.repeats > 0;
}

fn main() {
    let argv = args();
    var o = Opts {};
    if argv.len < 2 || !parse_opts(argv, o) {
        write_stderr("usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]\n");
        exit(2);
    }
    let k = argv[1];
    var times: f64[>..] = [];
    var sum:   u8[>..]  = [];
    if !dispatch(k, o, times, sum) {
        write_stderr(str("unknown kernel ", k, "\n"));
        exit(2);
    }
    var line: u8[>..] = [];
    format(line, "{\"kernel\":\"", k, "\",\"size\":", o.size, ",\"threads\":", o.threads);
    format(line, ",\"checksum\":\"", sum, "\",\"times_ms\":[");
    for t, i in times {
        if i > 0 { line.push(','); }
        format_flt(line, t, 3);
    }
    var backend: u8[>..] = [];
    if !env("KERNELS_GOOSE_BACKEND", backend) { backend.append("unknown backend"); }
    format(line, "],\"runtime\":\"goose (", backend, ")\"}");
    print(line);
}
