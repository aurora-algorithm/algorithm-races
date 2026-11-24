// Race: bwt (go)
// Extracted from suite/go/puzzle_text.go — build and run via suite/run.py

package main
import (
	"cmp"
	"slices"
)

type bwtResult struct {
	n, rounds, primary int
	out                []byte
}

func bwt(dna []byte) bwtResult {
	text := append(slices.Clone(dna), '$')
	n := len(text)
	rank := make([]int, n)
	sa := make([]int, n)
	for i, ch := range text {
		rank[i] = int(ch)
		sa[i] = i
	}
	second := make([]int, n)
	k, rounds := 1, 0
	for {
		rounds++
		for i := range second {
			second[i] = -1
			if i+k < n {
				second[i] = rank[i+k]
			}
		}
		slices.SortFunc(sa, func(a, b int) int {
			return cmp.Or(cmp.Compare(rank[a], rank[b]), cmp.Compare(second[a], second[b]))
		})
		tmp := make([]int, n)
		for j := 1; j < n; j++ {
			a, b := sa[j-1], sa[j]
			tmp[b] = tmp[a]
			if rank[a] != rank[b] || second[a] != second[b] {
				tmp[b]++
			}
		}
		rank = tmp
		if rank[sa[n-1]] == n-1 {
			break
		}
		k *= 2
	}
	out := make([]byte, n)
	primary := 0
	for j, i := range sa {
		if i == 0 {
			primary = j
			out[j] = text[n-1]
		} else {
			out[j] = text[i-1]
		}
	}
	return bwtResult{n: n, rounds: rounds, primary: primary, out: out}
}
