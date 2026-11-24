// Race: csv (goose)
// Extracted from suite/goose/work.goose — build and run via suite/run.py

import std;
import os;
import csv;
import dictionary;

fn csv_orders(path: u8[:], out: u8[>..]&) {
    var text: u8[>..] = [];
    if !read_file(path, text) { abort(str("cannot read ", path)); }
    var totals = dictionary<i64, i64> {};
    var rows = 0;
    var cust = -1;
    var total = -1;
    let ok = csv::each_record(text) { fields =>
        if cust < 0 {
            cust = position(fields, "customer_id");
            total = position(fields, "total_cents");
            assert(cust >= 0 && total >= 0);
        } else if fields.len > 1 {
            rows++;
            let key, k_ok = parse_int(fields[cust]);
            let cents, c_ok = parse_int(fields[total]);
            assert(k_ok && c_ok);
            totals.update(key, 0) { it += cents; };
        }
    };
    if !ok { abort("malformed CSV"); }
    var sum = 0;
    var top = 0;
    var top_sum = -1;
    totals.each() { key, v =>
        sum += v;
        if v > top_sum || (v == top_sum && key < top) {
            top = key;
            top_sum = v;
        }
    };
    format(out, rows, ":", totals.count, ":", sum, ":", top, ":", top_sum);
}
