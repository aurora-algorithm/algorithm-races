// Race: spectral-norm (go)
// Extracted from suite/go/bg.go — build and run via suite/run.py

package main
import (
	"fmt"
	"math"
)

func evalA(i, j int) float64 {
	return 1.0 / float64((i+j)*(i+j+1)/2+i+1)
}

func mulAv(u []float64) []float64 {
	out := make([]float64, len(u))
	for i := range out {
		s := 0.0
		for j, uj := range u {
			s += evalA(i, j) * uj
		}
		out[i] = s
	}
	return out
}

func mulAtv(u []float64) []float64 {
	out := make([]float64, len(u))
	for i := range out {
		s := 0.0
		for j, uj := range u {
			s += evalA(j, i) * uj
		}
		out[i] = s
	}
	return out
}

func mulAtAv(u []float64) []float64 {
	return mulAtv(mulAv(u))
}

func spectralNorm(n int) string {
	u := make([]float64, n)
	for i := range u {
		u[i] = 1.0
	}
	v := u
	for range 10 {
		v = mulAtAv(u)
		u = mulAtAv(v)
	}
	var vBv, vv float64
	for i := range n {
		vBv += u[i] * v[i]
		vv += v[i] * v[i]
	}
	return fmt.Sprintf("%.9f", math.Sqrt(vBv/vv))
}
