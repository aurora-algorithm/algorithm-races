// Race: dijkstra (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"container/heap"
	"fmt"
	"math"
)

type graphInput struct {
	to     []int
	weight []int64
}

type item struct {
	dist int64
	node int
}

// minHeap is the container/heap interface over a slice of items.
type minHeap []item

func (h minHeap) Len() int           { return len(h) }

func (h minHeap) Less(i, j int) bool { return h[i].dist < h[j].dist }

func (h minHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }

func (h *minHeap) Push(x any)        { *h = append(*h, x.(item)) }

func (h *minHeap) Pop() any {
	old := *h
	last := old[len(old)-1]
	*h = old[:len(old)-1]
	return last
}

func dijkstra(n int, g graphInput) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &minHeap{{dist: 0, node: 0}}
	for h.Len() > 0 {
		top := heap.Pop(h).(item)
		if top.dist > dist[top.node] {
			continue
		}
		for e := 4 * top.node; e < 4*top.node+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				heap.Push(h, item{dist: nd, node: v})
			}
		}
	}
	reached, total, longest := 0, int64(0), int64(0)
	for _, d := range dist {
		if d < math.MaxInt64 {
			reached++
			total += d
			longest = max(longest, d)
		}
	}
	return fmt.Sprintf("%d:%d:%d", reached, total, longest)
}

// less orders heap entries for binHeap.
type less[T any] interface {
	less(other T) bool
}

func (a item) less(b item) bool { return a.dist < b.dist }
