// Race: cycle (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
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

func cycleStarts(n int) []int64 {
	rng := newRNG()
	starts := make([]int64, n)
	for i := range starts {
		starts[i] = int64(rng.Next() % cycleMod)
	}
	return starts
}

func cycleStep(x int64) int64 {
	return (x*x + cycleC) % cycleMod
}

func cycle(starts []int64) string {
	totalMu, totalLam, longest := 0, 0, 0
	for _, x0 := range starts {
		tortoise := cycleStep(x0)
		hare := cycleStep(tortoise)
		for tortoise != hare {
			tortoise = cycleStep(tortoise)
			hare = cycleStep(cycleStep(hare))
		}
		mu := 0
		tortoise = x0
		for tortoise != hare {
			tortoise = cycleStep(tortoise)
			hare = cycleStep(hare)
			mu++
		}
		lam := 1
		hare = cycleStep(tortoise)
		for tortoise != hare {
			hare = cycleStep(hare)
			lam++
		}
		totalMu += mu
		totalLam += lam
		longest = max(longest, mu+lam)
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(starts), totalMu, totalLam, longest)
}
