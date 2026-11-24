// Race: rain-water (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"container/heap"
	"fmt"
	"math"
)

// packedHeap is a min-heap of packed int64 keys for container/heap.
type packedHeap []int64

func (h packedHeap) Len() int           { return len(h) }

func (h packedHeap) Less(i, j int) bool { return h[i] < h[j] }

func (h packedHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }

func (h *packedHeap) Push(x any)        { *h = append(*h, x.(int64)) }

func (h *packedHeap) Pop() any {
	old := *h
	x := old[len(old)-1]
	*h = old[:len(old)-1]
	return x
}

func terrain(side int) []int64 {
	rng := newRNG()
	heights := make([]int64, side*side)
	for i := range heights {
		heights[i] = int64(rng.Next() % 1000)
	}
	return heights
}

func rainWater(heights []int64) string {
	n := len(heights)
	side := int(math.Sqrt(float64(n)))
	seen := make([]bool, n)
	var queue packedHeap
	for idx := range n {
		r, c := idx/side, idx%side
		if r == 0 || r == side-1 || c == 0 || c == side-1 {
			seen[idx] = true
			queue = append(queue, heights[idx]*int64(n)+int64(idx))
		}
	}
	heap.Init(&queue)
	water, wet := int64(0), 0
	for queue.Len() > 0 {
		key := heap.Pop(&queue).(int64)
		level, idx := key/int64(n), int(key%int64(n))
		r, c := idx/side, idx%side
		for _, nb := range [4][2]int{{r - 1, c}, {r + 1, c}, {r, c - 1}, {r, c + 1}} {
			nr, nc := nb[0], nb[1]
			if nr < 0 || nr >= side || nc < 0 || nc >= side {
				continue
			}
			nidx := nr*side + nc
			if seen[nidx] {
				continue
			}
			seen[nidx] = true
			h := heights[nidx]
			if h < level {
				water += level - h
				wet++
				h = level
			}
			heap.Push(&queue, h*int64(n)+int64(nidx))
		}
	}
	return fmt.Sprintf("%d:%d:%d", side, water, wet)
}
