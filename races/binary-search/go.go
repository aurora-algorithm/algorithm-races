// Race: binary-search (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type searchInput struct {
	a, queries []int64
}

func sortedArray(n int) searchInput {
	rng := newRNG()
	in := searchInput{a: make([]int64, n), queries: make([]int64, n)}
	v := int64(0)
	for i := range in.a {
		v += 1 + int64(rng.Next()%4)
		in.a[i] = v
	}
	for i := range in.queries {
		in.queries[i] = int64(rng.Next()) % (v + 1)
	}
	return in
}

func binarySearch(in searchInput) string {
	n := len(in.a)
	hits, total := 0, 0
	for _, q := range in.queries {
		lo, hi := 0, n
		for lo < hi {
			mid := (lo + hi) / 2
			if in.a[mid] < q {
				lo = mid + 1
			} else {
				hi = mid
			}
		}
		if lo < n && in.a[lo] == q {
			hits++
		}
		total += lo
	}
	return fmt.Sprintf("%d:%d", hits, total)
}

func binarySearchKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(sortedArray(c.size)), binarySearch)
	return out, times, nil
}
