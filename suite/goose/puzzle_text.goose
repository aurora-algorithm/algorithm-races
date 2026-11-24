// The string kernels of the puzzle group (kernels/SPEC.md section 13): palindrome, bwt, aho-corasick, huffman.

import std;
import gen;

// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
fn binary_text(n: i64) -> u8[>..] {
    var r = xorshift();
    var out: u8[>..] = [];
    var v:   u32     = 0;
    for i in n {
        if i % 32 == 0 { v = r.next(); }
        out.push(if v & 1 != 0 { 'b' } else { 'a' });
        v >>= 1;
    }
    return out;
}

fn palindrome(text: u8[:], out: u8[>..]&) {
    var t: u8[>..] = [];
    t.push('^');
    t.push('#');
    for c in text {
        t.push(c);
        t.push('#');
    }
    t.push('$');
    var p: i64[>..] = [];
    p.resize(t.len, 0);
    var center = 0;
    var right = 0;
    for i in 1..t.len - 1 {
        if i < right { p[i] = min(right - i, p[2 * center - i]); }
        while t[i + 1 + p[i]] == t[i - 1 - p[i]] { p[i]++; }
        if i + p[i] > right {
            center = i;
            right = i + p[i];
        }
    }
    var longest = 0;
    var best = 0;
    var count = 0;
    for radius, i in p {
        count += (radius + 1) / 2;
        if radius > longest {
            longest = radius;
            best = i;
        }
    }
    format(out, text.len, ":", longest, ":", (best - 1 - longest) / 2, ":", count);
}

let DNA = "ACGT";

// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
fn dna_into(r: XorShift32&, n: i64, out: u8[>..]&) {
    var v: u32 = 0;
    for i in n {
        if i % 16 == 0 { v = r.next(); }
        out.push(DNA[v & 3]);
        v >>= 2;
    }
}

fn dna_text(n: i64) -> u8[>..] {
    var r = xorshift();
    var out: u8[>..] = [];
    dna_into(r, n, out);
    return out;
}

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

let AHO_PATTERNS = 10000;

// Patterns as inline strings (6 to 12 letters), then the text: a 1 MiB block repeated.
fn aho_input(n: i64, patterns: u8[..12][>..]&, text: u8[>..]&) {
    var r = xorshift();
    for i in AHO_PATTERNS {
        let p .= patterns.push([]);
        let len = 6 + r.next() % 7;
        for j in len { p.push(DNA[r.next() % 4]); }
    }
    dna_into(r, ONE_MIB, text);
    for i in 1..n / ONE_MIB { text.append(text[..ONE_MIB]); }
}

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

// A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
fn skewed_text(n: i64) -> u8[>..] {
    var r = xorshift();
    var out: u8[>..] = [];
    for i in ONE_MIB {
        let a = r.next() % 26;
        let b = r.next() % 26;
        out.push((97 + a * b / 26) as! u8);
    }
    for i in 1..n / ONE_MIB { out.append(out[..ONE_MIB]); }
    return out;
}

struct Pending { node: i64, code: i64, len: i64 }

// Writes "n:bits:" to out and the code to encoded, and returns 1 when decoding gives the text back; the caller
// finishes the checksum with fnv1a(encoded) after the timer.
fn huffman(text: u8[:], encoded: u8[>..]&, out: u8[>..]&) -> i64 {
    var freq: i64[256] = [0; 256];
    for b in text { freq[b]++; }
    var heap: i64[>..<] = [];
    for f, sym in freq {
        if f > 0 { heap.push(f * 1024 + sym); }
    }
    heap.make_heap();
    var left:  i64[512] = [0; 512];
    var right: i64[512] = [0; 512];
    var next_id = 256;
    while heap.len > 1 {
        let a = heap.heap_pop();
        let b = heap.heap_pop();
        left[next_id] = a % 1024;
        right[next_id] = b % 1024;
        heap.heap_push((a / 1024 + b / 1024) * 1024 + next_id);
        next_id++;
    }
    let root = heap[0] % 1024;
    var codes:   i64[256]      = [0; 256];
    var lengths: i64[256]      = [0; 256];
    var stack:   Pending[>..<] = [Pending { root, 0, 0 }];
    while stack.len > 0 {
        let p = stack.pop();
        if p.node < 256 {
            codes[p.node] = p.code;
            lengths[p.node] = p.len;
        } else {
            stack.push(Pending { left[p.node], p.code << 1, p.len + 1 });
            stack.push(Pending { right[p.node], (p.code << 1) | 1, p.len + 1 });
        }
    }
    var acc = 0;
    var nbits = 0;
    var bits = 0;
    for b in text {
        acc = (acc << lengths[b]) | codes[b];
        nbits += lengths[b];
        bits += lengths[b];
        while nbits >= 8 {
            nbits -= 8;
            encoded.push(((acc >> nbits) & 0xff) as! u8);
        }
        acc &= (1 << nbits) - 1;
    }
    if nbits > 0 { encoded.push(((acc << (8 - nbits)) & 0xff) as! u8); }
    var decoded: u8[>..] = [];
    decoded.resize(text.len, 0);
    var node = root;
    var k = 0;
    for byte in encoded {
        var shift = 7;
        while shift >= 0 {
            node = if (byte >> shift) & 1 != 0 { right[node] } else { left[node] };
            if node < 256 {
                decoded[k] = node as! u8;
                k++;
                node = root;
                if k == text.len { break; }
            }
            shift--;
        }
    }
    format(out, text.len, ":", bits, ":");
    return if decoded == text { 1 } else { 0 };
}
