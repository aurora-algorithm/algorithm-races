// Race: spectral-norm (goose)
// Extracted from suite/goose/bg.goose — build and run via suite/run.py

import std;
import math;

fn eval_a(i: i64, j: i64) -> f64 { 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1) }

fn mul_av(u: f64[:], out: f64[:]) {
    let n = u.len;
    for i in n {
        var s = 0.0;
        for j in n { s += eval_a(i, j) * u[j]; }
        out[i] = s;
    }
}

fn mul_atv(u: f64[:], out: f64[:]) {
    let n = u.len;
    for i in n {
        var s = 0.0;
        for j in n { s += eval_a(j, i) * u[j]; }
        out[i] = s;
    }
}

fn mul_atav(u: f64[:], out: f64[:], tmp: f64[:]) {
    mul_av(u, tmp);
    mul_atv(tmp, out);
}

fn spectral_norm(n: i64, out: u8[>..]&) {
    var u:   f64[>..] = [];
    var v:   f64[>..] = [];
    var tmp: f64[>..] = [];
    u.resize(n, 1.0);
    v.resize(n, 0.0);
    tmp.resize(n, 0.0);
    for i in 10 {
        mul_atav(u, v, tmp);
        mul_atav(v, u, tmp);
    }
    var vbv = 0.0;
    var vv = 0.0;
    for i in n {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    format_flt(out, sqrt(vbv / vv), 9);
}
