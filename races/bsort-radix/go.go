// Race: bsort-int.go.zermelo.Sort (go)
// Extracted from suite/go/ways.go — build and run via suite/run.py

package main
import (
	"slices"
	"sort"
)

func waySort[T any](input func(int) []T, run func([]T), check func([]T) string) kernel {
	return func(c config) (string, []float64, error) {
		base := input(c.size)
		sorted := func(a []T) []T {
			run(a)
			return a
		}
		out, times := measure(c.repeats, func() []T { return slices.Clone(base) }, sorted)
		return check(out), times, nil
	}
}

// sortIntsKernel: sort.Ints takes []int (64-bit on amd64 and arm64); the conversion from []int64 is untimed.
func sortIntsKernel(input func(int) []int64) kernel {
	return func(c config) (string, []float64, error) {
		base := input(c.size)
		fresh := func() []int {
			a := make([]int, len(base))
			for i, v := range base {
				a[i] = int(v)
			}
			return a
		}
		sorted := func(a []int) []int {
			sort.Ints(a)
			return a
		}
		out, times := measure(c.repeats, fresh, sorted)
		back := make([]int64, len(out))
		for i, v := range out {
			back[i] = int64(v)
		}
		return sortedChecksum(back), times, nil
	}
}
