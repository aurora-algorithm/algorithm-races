// Race: bwt (goose)
// Extracted from suite/goose/puzzle_text.goose — build and run via suite/run.py

import std;
import gen;

// Writes "n:rounds:primary:" to out and the transform to result; the caller adds fnv1a(result) after the timer.
fn bwt(dna: u8[:], result: u8[>..]&, out: u8[>..]&) {
    let n = dna.len + 1;
    var rank:   i64[>..] = [];
    var second: i64[>..] = [];
    var tmp:    i64[>..] = [];
    var sa:     i64[>..] = [];
    for c in dna { rank.push(c); }
    rank.push('$');
    for i in n { sa.push(i); }
    second.resize(n, 0);
    tmp.resize(n, 0);
    var k = 1;
    var rounds = 0;
    loop {
        rounds++;
        for i in n { second[i] = if i + k < n { rank[i + k] } else { -1 }; }
        sort(sa) { a, b => rank[a] < rank[b] || (rank[a] == rank[b] && second[a] < second[b]) };
        tmp[sa[0]] = 0;
        for j in 1..n {
            let a = sa[j - 1];
            let b = sa[j];
            let differs = rank[a] != rank[b] || second[a] != second[b];
            tmp[b] = if differs { tmp[a] + 1 } else { tmp[a] };
        }
        copy_into(rank, tmp);
        if rank[sa[n - 1]] == n - 1 { break; }
        k *= 2;
    }
    var primary = 0;
    for i, j in sa {
        if i == 0 {
            primary = j;
            result.push('$');
        } else {
            result.push(dna[i - 1]);
        }
    }
    format(out, n, ":", rounds, ":", primary, ":");
}
