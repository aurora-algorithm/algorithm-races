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

func runQuicksort(a []int64) []int64 {
	quicksort(a, 0, len(a)-1)
	return a
}

func runMergesort(a []int64) []int64 {
	mergeSort(a, make([]int64, len(a)), 0, len(a))
	return a
}

func runHeapsort(a []int64) []int64 {
	heapsort(a)
	return a
}
