// Race: regex-log-alt (goose)
// Extracted from suite/goose/misc.goose — build and run via suite/run.py

import std;
import dictionary;
import regex;
import gen;

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
