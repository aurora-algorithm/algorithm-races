// Race: sort-int (go)
// Extracted from suite/go/main.go — build and run via suite/run.py

package main

import "slices"

func sortInt(a []int64) []int64 {
	slices.Sort(a)
	return a
}
