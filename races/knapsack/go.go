// Race: knapsack (go)
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

type thing struct {
	weight int
	value  int64
}

func items() []thing {
	rng := newRNG()
	out := make([]thing, knapsackItems)
	for i := range out {
		weight := 1 + int(rng.Next()%1000)
		out[i] = thing{weight: weight, value: 1 + int64(rng.Next()%1000)}
	}
	return out
}

func knapsack(capacity int, things []thing) string {
	best := make([]int64, capacity+1)
	for _, t := range things {
		for c := capacity; c >= t.weight; c-- {
			if best[c-t.weight]+t.value > best[c] {
				best[c] = best[c-t.weight] + t.value
			}
		}
	}
	return fmt.Sprintf("%d:%d", best[capacity], h31(best))
}

func knapsackKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(items()), func(t []thing) string { return knapsack(c.size, t) })
	return out, times, nil
}
