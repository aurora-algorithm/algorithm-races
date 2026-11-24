// Race: bsort-str-sorted.goose.sort (go)
// Extracted from suite/go/ways.go — build and run via suite/run.py

package main
import (
	"cmp"
	"slices"
)

func sortedCopy[T cmp.Ordered](a []T, reverse bool) []T {
	slices.Sort(a)
	if reverse {
		slices.Reverse(a)
	}
	return a
}

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
