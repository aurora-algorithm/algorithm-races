// Race: radixsort (go)
// Extracted from suite/go/sorts.go — build and run via suite/run.py

package main

func radixSort(a []int64) []int64 {
	buf := make([]int64, len(a))
	for shift := 0; shift < 32; shift += 8 {
		var count [256]int
		for _, v := range a {
			count[(v>>shift)&255]++
		}
		var pos [256]int
		total := 0
		for d := range 256 {
			pos[d] = total
			total += count[d]
		}
		for _, v := range a {
			d := (v >> shift) & 255
			buf[pos[d]] = v
			pos[d]++
		}
		a, buf = buf, a
	}
	return a
}
