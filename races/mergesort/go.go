// Race: mergesort (go)
// Extracted from suite/go/sorts.go — build and run via suite/run.py

package main

func mergeSort(a, aux []int64, lo, hi int) {
	if hi-lo < 2 {
		return
	}
	mid := lo + (hi-lo)/2
	mergeSort(a, aux, lo, mid)
	mergeSort(a, aux, mid, hi)
	merge(a, aux, lo, mid, hi)
}

func merge(a, aux []int64, lo, mid, hi int) {
	copy(aux[lo:hi], a[lo:hi])
	i, j, k := lo, mid, lo
	for i < mid && j < hi {
		if aux[j] < aux[i] {
			a[k] = aux[j]
			j++
		} else {
			a[k] = aux[i]
			i++
		}
		k++
	}
	for i < mid {
		a[k] = aux[i]
		i++
		k++
	}
	for j < hi {
		a[k] = aux[j]
		j++
		k++
	}
}

func runMergesort(a []int64) []int64 {
	mergeSort(a, make([]int64, len(a)), 0, len(a))
	return a
}
