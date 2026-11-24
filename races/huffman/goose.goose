// Race: huffman (goose)
// Extracted from suite/goose/puzzle_text.goose — build and run via suite/run.py

import std;
import gen;

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
