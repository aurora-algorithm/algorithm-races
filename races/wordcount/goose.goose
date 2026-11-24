// Race: wordcount (goose)
// Extracted from suite/goose/work.goose — build and run via suite/run.py

import std;
import os;
import csv;
import dictionary;

fn wordcount(path: u8[:], out: u8[>..]&) {
    var text: u8[>..] = [];
    if !read_file(path, text) { abort(str("cannot read ", path)); }
    var counts = dictionary<u8[:], i64> {};
    var words = 0;
    each_word(text) { w =>
        words++;
        counts.update(w, 0) { it += 1; };
    };
    var top: u8[..16] = [];
    var top_count = 0;
    counts.each() { w, c =>
        if c > top_count || (c == top_count && compare(w, top) < 0) {
            top.clear();
            top.append(w);
            top_count = c;
        }
    };
    format(out, words, ":", counts.count, ":", top, ":", top_count);
}
