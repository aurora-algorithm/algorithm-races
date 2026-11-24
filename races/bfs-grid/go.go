// Race: bfs-grid (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

func grid(n int) []bool {
	rng := newRNG()
	walls := make([]bool, n*n)
	for i := range walls {
		walls[i] = rng.Next()%100 < 30
	}
	for _, cell := range []int{0, 1, n, n*n - n - 1, n*n - 2, n*n - 1} {
		walls[cell] = false
	}
	return walls
}

func bfsGrid(n int, walls []bool) string {
	dist := make([]int32, n*n)
	for i := range dist {
		dist[i] = -1
	}
	dist[0] = 0
	queue := []int{0}
	for head := 0; head < len(queue); head++ {
		cell := queue[head]
		d := dist[cell] + 1
		row, col := cell/n, cell%n
		neighbours := [4]struct {
			next int
			ok   bool
		}{{cell - n, row > 0}, {cell + n, row < n-1}, {cell - 1, col > 0}, {cell + 1, col < n-1}}
		for _, nb := range neighbours {
			if nb.ok && !walls[nb.next] && dist[nb.next] < 0 {
				dist[nb.next] = d
				queue = append(queue, nb.next)
			}
		}
	}
	reached, total := 0, int64(0)
	for _, d := range dist {
		if d >= 0 {
			reached++
			total += int64(d)
		}
	}
	return fmt.Sprintf("%d:%d:%d", reached, dist[n*n-1], total)
}

func bfsGridKernel(c config) (string, []float64, error) {
	walls := grid(c.size)
	out, times := measure(c.repeats, same(walls), func(w []bool) string { return bfsGrid(c.size, w) })
	return out, times, nil
}
