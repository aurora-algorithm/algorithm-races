// Race: mandelbrot (goose)
// Extracted from suite/goose/bg.goose — build and run via suite/run.py

import std;
import math;

fn mandelbrot(n: i64) -> u8[>..] {
    let row_bytes = (n + 7) / 8;
    var out: u8[>..] = [];
    out.resize(row_bytes * n, 0);
    for y in n {
        let ci = 2.0 * y / n - 1.0;
        let row = y * row_bytes;
        for x in n {
            let cr = 2.0 * x / n - 1.5;
            var zr = 0.0;
            var zi = 0.0;
            var tr = 0.0;
            var ti = 0.0;
            var i = 0;
            while i < 50 && tr + ti <= 4.0 {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i++;
            }
            if tr + ti <= 4.0 { out[row + (x >> 3)] |= (0x80 >> (x & 7)) as! u8; }
        }
    }
    return out;
}
