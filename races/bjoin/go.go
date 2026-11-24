// Race: bjoin.go.strings.Join (go)
// Extracted from suite/go/ways.go — build and run via suite/run.py

package main
import (
	"fmt"
	"strconv"
)

func joinParts(n int) []string {
	values := ints(n)
	parts := make([]string, n)
	for i, v := range values {
		parts[i] = strconv.FormatInt(v, 10)
	}
	return parts
}

func joinKernel(join func([]string) string) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(joinParts(c.size)), join)
		return fmt.Sprintf("%d:%d", len(out), fnv1a([]byte(out))), times, nil
	}
}
