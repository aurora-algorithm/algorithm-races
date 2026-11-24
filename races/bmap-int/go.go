// Race: bmap-int.go.map (go)
// Extracted from suite/go/ways.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type mapInput[K comparable] struct {
	inserts, lookups []K
}

func mapIntInput(n int) mapInput[int64] {
	inserts := ints(n)
	lookups := make([]int64, n)
	for i, k := range inserts {
		lookups[i] = k ^ int64(i&1)
	}
	return mapInput[int64]{inserts, lookups}
}

func countMap[K comparable](in mapInput[K]) string {
	counts := map[K]int64{}
	for _, k := range in.inserts {
		counts[k]++
	}
	hits := 0
	for _, k := range in.lookups {
		if _, ok := counts[k]; ok {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d", len(counts), hits)
}

func mapKernel[K comparable](input func(int) mapInput[K]) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(input(c.size)), countMap[K])
		return out, times, nil
	}
}
