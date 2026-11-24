// Race: lis (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
	"slices"
)

func lis(values []int64) string {
	var tails []int64
	for _, x := range values {
		i, _ := slices.BinarySearch(tails, x)
		if i == len(tails) {
			tails = append(tails, x)
		} else {
			tails[i] = x
		}
	}
	return fmt.Sprintf("%d:%d", len(tails), h31(tails))
}

func lisKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(ints(c.size)), lis)
	return out, times, nil
}
