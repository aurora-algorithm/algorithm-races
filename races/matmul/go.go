// Race: matmul (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main

type matrixInput struct {
	a, b []float64
	n    int
}

func matmul(m matrixInput) []float64 {
	n := m.n
	c := make([]float64, n*n)
	for i := range n {
		row := i * n
		for j := range n {
			s := 0.0
			for k := range n {
				s += m.a[row+k] * m.b[k*n+j]
			}
			c[row+j] = s
		}
	}
	return c
}
