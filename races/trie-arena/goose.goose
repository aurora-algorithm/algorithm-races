// Race: trie-arena (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

struct TrieNode { count: i64, children: TrieNode&<u32>?[26] }

fn trie(inserted: u8[..10][:], prefixes: u8[..4][:], out: u8[>..]&) {
    var pool: TrieNode[>..] = [];
    let root .= pool.push(TrieNode { count: 0, .. });
    var nodes = 0;
    for w in inserted {
        var node .= root;
        for ch in w {
            let i = ch - 'a';
            let child = node.children[i];
            if child {
                child.count += 1;
                node .= child;
            } else {
                let fresh .= pool.push(TrieNode { count: 1, .. });
                nodes++;
                node.children[i] .= fresh;
                node .= fresh;
            }
        }
    }
    var total = 0;
    for p in prefixes {
        var node: TrieNode? .= root;
        for ch in p {
            guard node else { break; }
            node .= node.children[ch - 'a'];
        }
        if node { total += node.count; }
    }
    format(out, nodes, ":", total);
}
