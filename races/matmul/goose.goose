// Race: matmul (goose)
// Extracted from suite/goose/misc.goose — build and run via suite/run.py

import std;
import dictionary;
import regex;
import gen;

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
