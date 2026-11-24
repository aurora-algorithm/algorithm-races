package main

import "sync"

func chunkBounds(n, threads int) []int {
	bounds := make([]int, threads+1)
	for t := range bounds {
		bounds[t] = t * n / threads
	}
	return bounds
}

// mergesortMT sorts T chunks in parallel, then merges neighbouring runs pairwise, one goroutine per merge,
// until one run is left. With an odd number of runs the last one waits for the next round.
func mergesortMT(a []int64, threads int) []int64 {
	aux := make([]int64, len(a))
	bounds := chunkBounds(len(a), threads)
	var wg sync.WaitGroup
	for t := range threads {
		wg.Go(func() { mergeSort(a, aux, bounds[t], bounds[t+1]) })
	}
	wg.Wait()
	for len(bounds) > 2 {
		next := make([]int, 0, len(bounds)/2+1)
		for p := 0; p+1 < len(bounds); p += 2 {
			if p+2 < len(bounds) {
				wg.Go(func() { merge(a, aux, bounds[p], bounds[p+1], bounds[p+2]) })
			}
			next = append(next, bounds[p])
		}
		wg.Wait()
		bounds = append(next, bounds[len(bounds)-1])
	}
	return a
}

func mandelbrotMT(n, threads int) []byte {
	width := rowBytes(n)
	bitmap := make([]byte, n*width)
	var wg sync.WaitGroup
	for t := range threads {
		wg.Go(func() {
			for y := t; y < n; y += threads {
				mandelbrotRow(n, y, bitmap[y*width:(y+1)*width])
			}
		})
	}
	wg.Wait()
	return bitmap
}

func binaryTreesMT(n, threads int) string {
	depthRun := func(depth, iterations int) int {
		share := iterations / threads
		checks := make([]int, threads)
		var wg sync.WaitGroup
		for t := range threads {
			wg.Go(func() { checks[t] = depthChecks(depth, share) })
		}
		wg.Wait()
		total := 0
		for _, c := range checks {
			total += c
		}
		return total
	}
	return binaryTreesLines(n, depthRun)
}
