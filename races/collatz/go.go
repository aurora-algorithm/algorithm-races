// Race: collatz (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
)

func collatz(n int) string {
	cache := make([]int32, n)
	cache[1] = 1
	best, bestLen, total := 1, int32(1), int64(1)
	for start := 2; start < n; start++ {
		x := int64(start)
		steps := int32(0)
		for x >= int64(start) {
			if x%2 == 0 {
				x /= 2
			} else {
				x = 3*x + 1
			}
			steps++
		}
		length := steps + cache[x]
		cache[start] = length
		total += int64(length)
		if length > bestLen {
			best, bestLen = start, length
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d", n, best, bestLen, total)
}
