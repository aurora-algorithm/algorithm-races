// Race: window-max (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"strconv"
	"strings"
)

var windows = []int{10, 100, 1000, 10000, 100000}

func windowMax(values []int64) string {
	n := len(values)
	dq := make([]int, n)
	hashes := make([]string, 0, len(windows))
	for _, w := range windows {
		head, tail := 0, 0
		var h uint32
		for i, v := range values {
			for tail > head && values[dq[tail-1]] <= v {
				tail--
			}
			dq[tail] = i
			tail++
			if dq[head] <= i-w {
				head++
			}
			if i >= w-1 {
				h = h*31 + uint32(values[dq[head]])
			}
		}
		hashes = append(hashes, formatHash(h))
	}
	return strconv.Itoa(n) + ":" + strings.Join(hashes, ":")
}
