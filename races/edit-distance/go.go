// Race: edit-distance (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

const (
	knapsackItems = 1000
	patternLen    = 8
	acgt          = "acgt"
)

func dna(length, count int) []string {
	rng := newRNG()
	out := make([]string, count)
	for i := range out {
		b := make([]byte, length)
		for j := range b {
			b[j] = acgt[rng.Next()%4]
		}
		out[i] = string(b)
	}
	return out
}

func editDistance(ab []string) string {
	a, b := ab[0], ab[1]
	prev := make([]int, len(b)+1)
	cur := make([]int, len(b)+1)
	for j := range prev {
		prev[j] = j
	}
	for i := range len(a) {
		cur[0] = i + 1
		for j := range len(b) {
			cost := 1
			if a[i] == b[j] {
				cost = 0
			}
			cur[j+1] = min(prev[j+1]+1, cur[j]+1, prev[j]+cost)
		}
		prev, cur = cur, prev
	}
	return fmt.Sprint(prev[len(b)])
}

func editDistanceKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(dna(c.size, 2)), editDistance)
	return out, times, nil
}
