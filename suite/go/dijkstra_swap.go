package main

import (
	"fmt"
	"math"
)

// swapHeap is container/heap's algorithm (heap.go's up, down, Push and Pop, swap by swap) with a type parameter in
// place of heap.Interface: no `any`, no interface calls, the same sift as container/heap (runtime go-swapheap). It
// separates what the interface and the boxes cost from what binHeap's hole-moving sift saves.
type swapHeap[T less[T]] struct{ items []T }

func (h *swapHeap[T]) push(x T) {
	h.items = append(h.items, x)
	h.up(len(h.items) - 1)
}

func (h *swapHeap[T]) pop() T {
	n := len(h.items) - 1
	h.items[0], h.items[n] = h.items[n], h.items[0]
	h.down(0, n)
	x := h.items[n]
	h.items = h.items[:n]
	return x
}

func (h *swapHeap[T]) up(j int) {
	for {
		i := (j - 1) / 2
		if i == j || !h.items[j].less(h.items[i]) {
			break
		}
		h.items[i], h.items[j] = h.items[j], h.items[i]
		j = i
	}
}

func (h *swapHeap[T]) down(i0, n int) {
	i := i0
	for {
		j1 := 2*i + 1
		if j1 >= n || j1 < 0 {
			break
		}
		j := j1
		if j2 := j1 + 1; j2 < n && h.items[j2].less(h.items[j1]) {
			j = j2
		}
		if !h.items[j].less(h.items[i]) {
			break
		}
		h.items[i], h.items[j] = h.items[j], h.items[i]
		i = j
	}
}

// dijkstraSwap is dijkstraTyped with swapHeap in place of binHeap.
func dijkstraSwap(n int, g graphInput) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &swapHeap[item]{items: []item{{dist: 0, node: 0}}}
	for len(h.items) > 0 {
		top := h.pop()
		if top.dist > dist[top.node] {
			continue
		}
		for e := 4 * top.node; e < 4*top.node+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				h.push(item{dist: nd, node: v})
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
