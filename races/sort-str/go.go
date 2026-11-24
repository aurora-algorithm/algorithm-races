// Race: sort-str (go)
// Extracted from suite/go/main.go — build and run via suite/run.py

package main

import "slices"

func sortStr(a []string) []string {
	slices.Sort(a)
	return a
}
