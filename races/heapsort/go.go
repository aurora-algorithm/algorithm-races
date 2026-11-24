// Race: heapsort (go)
// Extracted from suite/go/sorts.go — build and run via suite/run.py

package main

func siftDown(a []int64, root, end int) {
	for 2*root+1 < end {
		child := 2*root + 1
		if child+1 < end && a[child] < a[child+1] {
			child++
		}
		if a[root] < a[child] {
			a[root], a[child] = a[child], a[root]
			root = child
		} else {
			return
		}
	}
}

func heapsort(a []int64) {
	n := len(a)
	for start := n/2 - 1; start >= 0; start-- {
		siftDown(a, start, n)
	}
	for end := n - 1; end > 0; end-- {
		a[0], a[end] = a[end], a[0]
		siftDown(a, 0, end)
	}
}

func runHeapsort(a []int64) []int64 {
	heapsort(a)
	return a
}
