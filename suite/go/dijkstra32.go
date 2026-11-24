package main

import (
	"container/heap"
	"fmt"
	"math"
	"os"
	"runtime"
	"unsafe"
)

// graphInput32 is graphInput with C#'s types: 32-bit edge targets (int[] To), 64-bit weights (long[] Weight).
type graphInput32 struct {
	to     []int32
	weight []int64
}

// graph32 builds the same graph as graph, from the same random sequence.
func graph32(n int) graphInput32 {
	rng := newRNG()
	g := graphInput32{to: make([]int32, 4*n), weight: make([]int64, 4*n)}
	for u := range n {
		g.to[4*u] = int32((u + 1) % n)
		g.weight[4*u] = 1 + int64(rng.Next()%100)
		for e := 4*u + 1; e < 4*u+4; e++ {
			g.to[e] = int32(rng.Next() % uint32(n))
			g.weight[e] = 1 + int64(rng.Next()%100)
		}
	}
	return g
}

// item32 matches C#'s PriorityQueue<int, long> entry: a 32-bit node and a 64-bit priority (16 bytes, as item).
type item32 struct {
	dist int64
	node int32
}

func (a item32) less(b item32) bool { return a.dist < b.dist }

type minHeap32 []item32

func (h minHeap32) Len() int           { return len(h) }
func (h minHeap32) Less(i, j int) bool { return h[i].dist < h[j].dist }
func (h minHeap32) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }
func (h *minHeap32) Push(x any)        { *h = append(*h, x.(item32)) }
func (h *minHeap32) Pop() any {
	old := *h
	last := old[len(old)-1]
	*h = old[:len(old)-1]
	return last
}

// queue32 is the push/pop/size view dijkstraOn32 runs on, so the timed and counting runs share one loop.
type queue32 struct {
	push func(item32)
	pop  func() item32
	size func() int
}

func containerQueue32() queue32 {
	h := &minHeap32{}
	return queue32{
		push: func(x item32) { heap.Push(h, x) },
		pop:  func() item32 { return heap.Pop(h).(item32) },
		size: func() int { return h.Len() },
	}
}

func distChecksum(dist []int64) string {
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

// dijkstraHeap32 is dijkstra on graphInput32 with container/heap (runtime go-heap32).
func dijkstraHeap32(n int, g graphInput32) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &minHeap32{{dist: 0, node: 0}}
	for h.Len() > 0 {
		top := heap.Pop(h).(item32)
		if top.dist > dist[top.node] {
			continue
		}
		for e := 4 * int(top.node); e < 4*int(top.node)+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				heap.Push(h, item32{dist: nd, node: v})
			}
		}
	}
	return distChecksum(dist)
}

// dijkstraTyped32 is dijkstraTyped on graphInput32 (runtime go-typedheap32).
func dijkstraTyped32(n int, g graphInput32) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &binHeap[item32]{items: []item32{{dist: 0, node: 0}}}
	for len(h.items) > 0 {
		top := h.pop()
		if top.dist > dist[top.node] {
			continue
		}
		for e := 4 * int(top.node); e < 4*int(top.node)+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				h.push(item32{dist: nd, node: v})
			}
		}
	}
	return distChecksum(dist)
}

// dijkstraCounts32 is dijkstraCounts on graphInput32. It is never timed.
func dijkstraCounts32(n int, g graphInput32, typed bool) (pushes, pops, stale int, checksum string) {
	q := containerQueue32()
	if typed {
		h := &binHeap[item32]{}
		q = queue32{push: h.push, pop: h.pop, size: func() int { return len(h.items) }}
	}
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	q.push(item32{dist: 0, node: 0})
	pushes = 1
	for q.size() > 0 {
		top := q.pop()
		pops++
		if top.dist > dist[top.node] {
			stale++
			continue
		}
		for e := 4 * int(top.node); e < 4*int(top.node)+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				pushes++
				q.push(item32{dist: nd, node: v})
			}
		}
	}
	return pushes, pops, stale, distChecksum(dist)
}

// dijkstra32Kernel is dijkstraKernel for KERNELS_DIJKSTRA_HEAP=container32 and typed32, with the same extras
// (no export: the trace is the same graph and the same order as container/heap's).
func dijkstra32Kernel(c config, typed bool) (string, []float64, map[string]any, error) {
	g := graph32(c.size)
	search, heapName := dijkstraHeap32, "container/heap int32"
	if typed {
		search, heapName = dijkstraTyped32, "typed binHeap[item32]"
	}
	var before, after runtime.MemStats
	runtime.ReadMemStats(&before)
	out, times := measure(c.repeats, same(g), func(g graphInput32) string { return search(c.size, g) })
	runtime.ReadMemStats(&after)
	runs := uint64(c.repeats)
	extra := map[string]any{
		"heap":              heapName,
		"mallocs_per_run":   (after.Mallocs - before.Mallocs) / runs,
		"alloc_mb_per_run":  math.Round(float64(after.TotalAlloc-before.TotalAlloc)/float64(runs)/1e4) / 100,
		"gc_cycles_per_run": float64(after.NumGC-before.NumGC) / float64(runs),
		"entry_bytes":       unsafe.Sizeof(item32{}),
		"edge_target_bytes": unsafe.Sizeof(g.to[0]),
	}
	if os.Getenv("KERNELS_DIJKSTRA_COUNTS") == "1" {
		pushes, pops, stale, sum := dijkstraCounts32(c.size, g, typed)
		if sum != out {
			return "", nil, nil, fmt.Errorf("counting run checksum %s, timed run %s", sum, out)
		}
		extra["pushes"], extra["pops"], extra["stale_pops"] = pushes, pops, stale
	}
	if path := os.Getenv("KERNELS_DIJKSTRA_PROFILE"); path != "" {
		if err := dijkstraProfile(path, c.size, g, search); err != nil {
			return "", nil, nil, fmt.Errorf("cpu profile: %w", err)
		}
	}
	return out, times, extra, nil
}
