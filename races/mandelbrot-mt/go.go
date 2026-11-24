// Race: mandelbrot-mt (go)
// Extracted from suite/go/mt.go — build and run via suite/run.py

package main
import "sync"

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
