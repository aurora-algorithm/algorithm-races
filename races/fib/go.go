// Race: fib (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main

func fib(n int) int {
	if n < 2 {
		return n
	}
	return fib(n-1) + fib(n-2)
}
