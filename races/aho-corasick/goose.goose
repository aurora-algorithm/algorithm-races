// Race: aho-corasick (goose)
// Extracted from suite/goose/puzzle_text.goose — build and run via suite/run.py

import std;
import gen;

let DNA = "ACGT";

fn aho_corasick(patterns: u8[..12][:], text: u8[:], out: u8[>..]&) {
    var code: i64[256] = [0; 256];
    for ch, i in DNA { code[ch] = i; }
    var goto:    i64[>..] = [-1, -1, -1, -1];
    var hits_at: i64[>..] = [0];
    for pattern in patterns {
        var s = 0;
        for ch in pattern {
            let edge = s * 4 + code[ch];
            if goto[edge] == -1 {
                goto[edge] = hits_at.len;
                push_n(goto, -1, 4);
                hits_at.push(0);
            }
            s = goto[edge];
        }
        hits_at[s]++;
    }
    var fail: i64[>..] = [];
    fail.resize(hits_at.len, 0);
    var queue: i64[>..] = [];
    for c in 4 {
        if goto[c] == -1 { goto[c] = 0; } else { queue.push(goto[c]); }
    }
    var head = 0;
    while head < queue.len {
        let s = queue[head];
        head++;
        hits_at[s] += hits_at[fail[s]];
        for c in 4 {
            let t = goto[s * 4 + c];
            if t == -1 {
                goto[s * 4 + c] = goto[fail[s] * 4 + c];
            } else {
                fail[t] = goto[fail[s] * 4 + c];
                queue.push(t);
            }
        }
    }
    var s = 0;
    var matches = 0;
    var hits = 0;
    for ch in text {
        s = goto[s * 4 + code[ch]];
        let found = hits_at[s];
        if found > 0 {
            matches += found;
            hits++;
        }
    }
    format(out, patterns.len, ":", hits_at.len, ":", matches, ":", hits);
}
