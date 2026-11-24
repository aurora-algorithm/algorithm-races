// Race: trie (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type trieInput struct {
	inserted, prefixes []string
}

func word(rng *XorShift32, minLen, spread uint32) string {
	b := make([]byte, minLen+rng.Next()%spread)
	for i := range b {
		b[i] = byte('a' + rng.Next()%26)
	}
	return string(b)
}

func words(n int) trieInput {
	rng := newRNG()
	in := trieInput{inserted: make([]string, n), prefixes: make([]string, n)}
	for i := range in.inserted {
		in.inserted[i] = word(rng, 3, 8)
	}
	for i := range in.prefixes {
		in.prefixes[i] = word(rng, 1, 4)
	}
	return in
}

type trieNode struct {
	count    int
	children [26]*trieNode
}

func trie(in trieInput) string {
	root := &trieNode{}
	nodes := 0
	for _, w := range in.inserted {
		n := root
		for i := range len(w) {
			ch := w[i] - 'a'
			if n.children[ch] == nil {
				n.children[ch] = &trieNode{}
				nodes++
			}
			n = n.children[ch]
			n.count++
		}
	}
	total := 0
	for _, p := range in.prefixes {
		n := root
		for i := 0; i < len(p) && n != nil; i++ {
			n = n.children[p[i]-'a']
		}
		if n != nil {
			total += n.count
		}
	}
	return fmt.Sprintf("%d:%d", nodes, total)
}

func trieKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(words(c.size)), trie)
	return out, times, nil
}
