// Race: mandelbrot (go)
// Extracted from suite/go/bg.go — build and run via suite/run.py

package main

func mandelbrotRow(n, y int, row []byte) {
	ci := 2.0*float64(y)/float64(n) - 1.0
	for x := range n {
		cr := 2.0*float64(x)/float64(n) - 1.5
		var zr, zi, tr, ti float64
		i := 0
		for i < 50 && tr+ti <= 4.0 {
			zi = 2.0*zr*zi + ci
			zr = tr - ti + cr
			tr = zr * zr
			ti = zi * zi
			i++
		}
		if tr+ti <= 4.0 {
			row[x>>3] |= 0x80 >> (x & 7)
		}
	}
}

func rowBytes(n int) int {
	return (n + 7) / 8
}

func mandelbrot(n int) []byte {
	width := rowBytes(n)
	bitmap := make([]byte, n*width)
	for y := range n {
		mandelbrotRow(n, y, bitmap[y*width:(y+1)*width])
	}
	return bitmap
}
