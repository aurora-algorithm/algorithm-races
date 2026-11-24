// Race: hashmap (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type hashmapInput struct {
	inserts, lookups []int64
}

func hashmap(in hashmapInput) string {
	counts := map[int64]int64{}
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
