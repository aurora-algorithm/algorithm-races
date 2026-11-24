// Race: sieve (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main
import (
	"strconv"
)

func sieve(n int) string {
	composite := make([]bool, n+1)
	for i := 2; i*i <= n; i++ {
		if !composite[i] {
			for j := i * i; j <= n; j += i {
				composite[j] = true
			}
		}
	}
	count := 0
	for i := 2; i <= n; i++ {
		if !composite[i] {
			count++
		}
	}
	return strconv.Itoa(count)
}
