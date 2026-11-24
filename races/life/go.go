// Race: life (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
	"slices"
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

type lifeInput struct {
	grid []byte
	gens int
}

type lifeResult struct {
	gens int
	grid []byte
}

func newLifeInput(gens int) lifeInput {
	rng := newRNG()
	grid := make([]byte, lifeSide*lifeSide)
	for i := range grid {
		if rng.Next()%4 == 0 {
			grid[i] = 1
		}
	}
	return lifeInput{grid: grid, gens: gens}
}

func life(in lifeInput) lifeResult {
	w := lifeSide
	cur := slices.Clone(in.grid)
	nxt := make([]byte, len(cur))
	for range in.gens {
		for y := range w {
			up := (y + w - 1) % w * w
			row := y * w
			down := (y + 1) % w * w
			for x := range w {
				left := (x + w - 1) % w
				right := (x + 1) % w
				n := cur[up+left] + cur[up+x] + cur[up+right] +
					cur[row+left] + cur[row+right] +
					cur[down+left] + cur[down+x] + cur[down+right]
				if n == 3 || (n == 2 && cur[row+x] == 1) {
					nxt[row+x] = 1
				} else {
					nxt[row+x] = 0
				}
			}
		}
		cur, nxt = nxt, cur
	}
	return lifeResult{gens: in.gens, grid: cur}
}

func lifeChecksum(r lifeResult) string {
	alive := 0
	for _, cell := range r.grid {
		alive += int(cell)
	}
	return fmt.Sprintf("%d:%d:%d", r.gens, alive, fnv1a(r.grid))
}
