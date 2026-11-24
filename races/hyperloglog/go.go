// Race: hyperloglog (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
	"math"
	"math/bits"
)

const (
	oneMiB = 1 << 20

	sudokuClues = 26
	dlxColumns  = 324

	bloomBitsPerKey = 10
	bloomHashes     = 7
	golden          = 0x9e3779b9

	hllP = 14
	hllM = 1 << hllP

	skipMaxLevel  = 24
	skipLevelSeed = 88675123

	cycleMod = 67108859
	cycleC   = 12345

	ahoPatterns = 10000
	lifeSide    = 512
	dnaLetters  = "ACGT"
)

// fmix32 is MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
func fmix32(h uint32) uint32 {
	h ^= h >> 16
	h *= 0x85ebca6b
	h ^= h >> 13
	h *= 0xc2b2ae35
	h ^= h >> 16
	return h
}

func hyperloglog(n int) string {
	rng := newRNG()
	universe := uint32(max(n/4, 1))
	registers := make([]byte, hllM)
	for range n {
		h := fmix32(rng.Next() % universe)
		w := h << hllP
		rank := byte(32 - hllP + 1)
		if w != 0 {
			rank = byte(bits.LeadingZeros32(w) + 1)
		}
		idx := h >> (32 - hllP)
		if rank > registers[idx] {
			registers[idx] = rank
		}
	}
	z := 0.0
	zeros := 0
	values := make([]int64, hllM)
	for i, r := range registers {
		z += math.Ldexp(1, -int(r))
		if r == 0 {
			zeros++
		}
		values[i] = int64(r)
	}
	m := float64(hllM)
	estimate := 0.7213 / (1.0 + 1.079/m) * m * m / z
	if estimate <= 2.5*m && zeros > 0 {
		estimate = m * math.Log(m/float64(zeros))
	}
	return fmt.Sprintf("%d:%d:%d", n, int64(math.Floor(estimate+0.5)), h31(values))
}
