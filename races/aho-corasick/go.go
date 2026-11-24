// Race: aho-corasick (go)
// Extracted from suite/go/puzzle_text.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type ahoInput struct {
	patterns [][]byte
	text     []byte
}

func ahoCorasick(in ahoInput) string {
	var code [256]int
	for i := range len(dnaLetters) {
		code[dnaLetters[i]] = i
	}
	edges := []int{-1, -1, -1, -1}
	out := []int{0}
	for _, pattern := range in.patterns {
		s := 0
		for _, ch := range pattern {
			edge := s*4 + code[ch]
			if edges[edge] == -1 {
				edges[edge] = len(out)
				edges = append(edges, -1, -1, -1, -1)
				out = append(out, 0)
			}
			s = edges[edge]
		}
		out[s]++
	}
	fail := make([]int, len(out))
	var queue []int
	for c := range 4 {
		if edges[c] == -1 {
			edges[c] = 0
		} else {
			queue = append(queue, edges[c])
		}
	}
	for head := 0; head < len(queue); head++ {
		s := queue[head]
		out[s] += out[fail[s]]
		for c := range 4 {
			t := edges[s*4+c]
			if t == -1 {
				edges[s*4+c] = edges[fail[s]*4+c]
			} else {
				fail[t] = edges[fail[s]*4+c]
				queue = append(queue, t)
			}
		}
	}
	s, matches, hits := 0, 0, 0
	for _, ch := range in.text {
		s = edges[s*4+code[ch]]
		if found := out[s]; found > 0 {
			matches += found
			hits++
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(in.patterns), len(out), matches, hits)
}
