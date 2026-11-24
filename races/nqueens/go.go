// Race: nqueens (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main

type board struct {
	n              int
	cols, up, down []bool
}

func (b *board) place(row int) int {
	if row == b.n {
		return 1
	}
	count := 0
	for col := range b.n {
		u, d := row+col, row+b.n-1-col
		if !b.cols[col] && !b.up[u] && !b.down[d] {
			b.cols[col], b.up[u], b.down[d] = true, true, true
			count += b.place(row + 1)
			b.cols[col], b.up[u], b.down[d] = false, false, false
		}
	}
	return count
}

func nqueens(n int) int {
	b := &board{n: n, cols: make([]bool, n), up: make([]bool, 2*n-1), down: make([]bool, 2*n-1)}
	return b.place(0)
}
