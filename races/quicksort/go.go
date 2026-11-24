// Race: quicksort (go)
// Extracted from suite/go/sorts.go — build and run via suite/run.py

package main

func quicksort(a []int64, lo, hi int) {
	for lo < hi {
		pivot := a[lo+(hi-lo)/2]
		i, j := lo-1, hi+1
		for {
			i++
			for a[i] < pivot {
				i++
			}
			j--
			for a[j] > pivot {
				j--
			}
			if i >= j {
				break
			}
			a[i], a[j] = a[j], a[i]
		}
		if j-lo < hi-j {
			quicksort(a, lo, j)
			lo = j + 1
		} else {
			quicksort(a, j+1, hi)
			hi = j
		}
	}
}

func runQuicksort(a []int64) []int64 {
	quicksort(a, 0, len(a)-1)
	return a
}
