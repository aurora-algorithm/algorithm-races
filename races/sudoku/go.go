// Race: sudoku (go)
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

func shuffle[T any](rng *XorShift32, items []T) {
	for i := len(items) - 1; i > 0; i-- {
		j := int(rng.Next() % uint32(i+1))
		items[i], items[j] = items[j], items[i]
	}
}

func sudokuPuzzles(n int) [][]int {
	rng := newRNG()
	puzzles := make([][]int, 0, n)
	for range n {
		digits := []int{1, 2, 3, 4, 5, 6, 7, 8, 9}
		shuffle(rng, digits)
		var order [2][]int
		for axis := range order {
			bands := []int{0, 1, 2}
			shuffle(rng, bands)
			for _, band := range bands {
				inner := []int{0, 1, 2}
				shuffle(rng, inner)
				for _, i := range inner {
					order[axis] = append(order[axis], band*3+i)
				}
			}
		}
		rows, cols := order[0], order[1]
		cells := make([]int, 81)
		for i := range cells {
			cells[i] = i
		}
		shuffle(rng, cells)
		puzzle := make([]int, 81)
		for _, cell := range cells[:sudokuClues] {
			r, c := rows[cell/9], cols[cell%9]
			puzzle[cell] = digits[(r*3+r/3+c)%9]
		}
		puzzles = append(puzzles, puzzle)
	}
	return puzzles
}

// dlx is the exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
type dlx struct {
	left, right, up, down, col, row []int
	count                           []int
	solutions, tries                int
	chosen, first                   []int
}

func newDlx() *dlx {
	size := 1 + dlxColumns + 729*4
	d := &dlx{
		left:   make([]int, size),
		right:  make([]int, size),
		up:     make([]int, size),
		down:   make([]int, size),
		col:    make([]int, size),
		row:    make([]int, size),
		count:  make([]int, dlxColumns+1),
		chosen: make([]int, 81),
	}
	for i := range size {
		d.up[i], d.down[i], d.col[i] = i, i, i
	}
	for i := range dlxColumns + 1 {
		d.left[i], d.right[i] = i-1, i+1
	}
	d.left[0] = dlxColumns
	d.right[dlxColumns] = 0
	node := dlxColumns + 1
	for r := range 9 {
		for c := range 9 {
			box := (r/3)*3 + c/3
			for digit := range 9 {
				first := node
				for _, column := range []int{1 + r*9 + c, 82 + r*9 + digit, 163 + c*9 + digit, 244 + box*9 + digit} {
					d.col[node] = column
					d.row[node] = (r*9+c)*9 + digit
					d.up[node] = d.up[column]
					d.down[node] = column
					d.down[d.up[column]] = node
					d.up[column] = node
					d.count[column]++
					d.left[node] = node - 1
					d.right[node] = node + 1
					node++
				}
				d.left[first] = node - 1
				d.right[node-1] = first
			}
		}
	}
	return d
}

func (d *dlx) cover(c int) {
	d.right[d.left[c]] = d.right[c]
	d.left[d.right[c]] = d.left[c]
	for i := d.down[c]; i != c; i = d.down[i] {
		for j := d.right[i]; j != i; j = d.right[j] {
			d.down[d.up[j]] = d.down[j]
			d.up[d.down[j]] = d.up[j]
			d.count[d.col[j]]--
		}
	}
}

func (d *dlx) uncover(c int) {
	for i := d.up[c]; i != c; i = d.up[i] {
		for j := d.left[i]; j != i; j = d.left[j] {
			d.count[d.col[j]]++
			d.down[d.up[j]] = j
			d.up[d.down[j]] = j
		}
	}
	d.right[d.left[c]] = c
	d.left[d.right[c]] = c
}

func (d *dlx) chooseRow(node int) {
	for j := d.right[node]; j != node; j = d.right[j] {
		d.cover(d.col[j])
	}
}

func (d *dlx) unchooseRow(node int) {
	for j := d.left[node]; j != node; j = d.left[j] {
		d.uncover(d.col[j])
	}
}

func (d *dlx) search(depth int) {
	if d.right[0] == 0 {
		d.solutions++
		if d.solutions == 1 {
			d.first = slices.Clone(d.chosen[:depth])
		}
		return
	}
	best := d.right[0]
	for c := d.right[best]; c != 0; c = d.right[c] {
		if d.count[c] < d.count[best] {
			best = c
		}
	}
	d.cover(best)
	for r := d.down[best]; r != best; r = d.down[r] {
		d.tries++
		d.chosen[depth] = d.row[r]
		d.chooseRow(r)
		d.search(depth + 1)
		d.unchooseRow(r)
	}
	d.uncover(best)
}

func sudoku(puzzles [][]int) string {
	solutions, tries := 0, 0
	var h uint32
	for _, puzzle := range puzzles {
		d := newDlx()
		depth := 0
		for cell, digit := range puzzle {
			if digit != 0 {
				node := dlxColumns + 1 + (cell*9+digit-1)*4
				d.cover(d.col[node])
				d.chooseRow(node)
				d.chosen[depth] = cell*9 + digit - 1
				depth++
			}
		}
		d.search(depth)
		solutions += d.solutions
		tries += d.tries
		values := make([]int64, 82)
		values[0] = int64(h)
		for _, row := range d.first {
			values[1+row/9] = int64(row%9 + 1)
		}
		h = h31(values)
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(puzzles), solutions, tries, h)
}
