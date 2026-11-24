// Race: bmap-str.go.map (go)
// Extracted from suite/go/ways.go — build and run via suite/run.py

package main
import (
	"fmt"
	"strings"
)

type mapInput[K comparable] struct {
	inserts, lookups []K
}

// mapStrInput: even i looks up a key in the map, odd i the key without its last letter (almost never in it).
func mapStrInput(n int) mapInput[string] {
	inserts := randomStrings(n)
	lookups := make([]string, n)
	for i, k := range inserts {
		if i%2 == 0 {
			lookups[i] = k
		} else {
			lookups[i] = k[:len(k)-1]
		}
	}
	return mapInput[string]{inserts, lookups}
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

// mapStrKernel gives every repeat its own copies of the inserts and, separately, of the lookups (made untimed), as
// the languages whose strings cache their hash need; equal keys are never the same pointer.
func mapStrKernel(c config) (string, []float64, error) {
	in := mapStrInput(c.size)
	fresh := func() mapInput[string] {
		return mapInput[string]{cloneAll(in.inserts), cloneAll(in.lookups)}
	}
	out, times := measure(c.repeats, fresh, countMap[string])
	return out, times, nil
}

func cloneAll(keys []string) []string {
	out := make([]string, len(keys))
	for i, k := range keys {
		out[i] = strings.Clone(k)
	}
	return out
}
