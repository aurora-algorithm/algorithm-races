package main

import (
	"container/heap"
	"encoding/json"
	"fmt"
	"math"
	"os"
	"runtime/pprof"
	"strconv"
)

// quadHeap is binHeap with four children per entry, the arity of .NET's PriorityQueue.
type quadHeap[T less[T]] struct{ items []T }

func (h *quadHeap[T]) push(x T) {
	h.items = append(h.items, x)
	i := len(h.items) - 1
	for i > 0 {
		p := (i - 1) / 4
		if !x.less(h.items[p]) {
			break
		}
		h.items[i] = h.items[p]
		i = p
	}
	h.items[i] = x
}

func (h *quadHeap[T]) pop() T {
	top := h.items[0]
	last := h.items[len(h.items)-1]
	h.items = h.items[:len(h.items)-1]
	m := len(h.items)
	if m == 0 {
		return top
	}
	i := 0
	for {
		first := 4*i + 1
		if first >= m {
			break
		}
		c := first
		for k := first + 1; k < min(first+4, m); k++ {
			if h.items[k].less(h.items[c]) {
				c = k
			}
		}
		if !h.items[c].less(last) {
			break
		}
		h.items[i] = h.items[c]
		i = c
	}
	h.items[i] = last
	return top
}

// dijkstraTyped4 is dijkstraTyped with quadHeap (runtime go-typedheap4).
func dijkstraTyped4(n int, g graphInput) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &quadHeap[item]{items: []item{{dist: 0, node: 0}}}
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

// dijkstraCounts runs Dijkstra once more with the heap that heapName names and counts pushes, pops and stale pops
// (KERNELS_DIJKSTRA_COUNTS=1). It is never timed.
func dijkstraCounts(n int, g graphInput, heapName string) (pushes, pops, stale int, checksum string) {
	var push func(item)
	var pop func() item
	var size func() int
	switch heapName {
	case "typed binHeap[item]":
		h := &binHeap[item]{}
		push, pop, size = h.push, h.pop, func() int { return len(h.items) }
	case "typed quadHeap[item]":
		h := &quadHeap[item]{}
		push, pop, size = h.push, h.pop, func() int { return len(h.items) }
	case "typed swapHeap[item]":
		h := &swapHeap[item]{}
		push, pop, size = h.push, h.pop, func() int { return len(h.items) }
	default:
		h := &minHeap{}
		push = func(x item) { heap.Push(h, x) }
		pop = func() item { return heap.Pop(h).(item) }
		size = func() int { return h.Len() }
	}
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	push(item{dist: 0, node: 0})
	pushes = 1
	for size() > 0 {
		top := pop()
		pops++
		if top.dist > dist[top.node] {
			stale++
			continue
		}
		for e := 4 * top.node; e < 4*top.node+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				pushes++
				push(item{dist: nd, node: v})
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
	return pushes, pops, stale, fmt.Sprintf("%d:%d:%d", reached, total, longest)
}

// dijkstraProfile runs search `runs` more times under the CPU profiler, after the timed repeats
// (KERNELS_DIJKSTRA_PROFILE=<file>, KERNELS_DIJKSTRA_PROFILE_RUNS, default 5).
func dijkstraProfile[G any](path string, n int, g G, search func(int, G) string) error {
	runs := 5
	if s := os.Getenv("KERNELS_DIJKSTRA_PROFILE_RUNS"); s != "" {
		v, err := strconv.Atoi(s)
		if err != nil || v <= 0 {
			return fmt.Errorf("KERNELS_DIJKSTRA_PROFILE_RUNS=%q: want a positive integer", s)
		}
		runs = v
	}
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	if err := pprof.StartCPUProfile(f); err != nil {
		_ = f.Close()
		return err
	}
	for range runs {
		search(n, g)
	}
	pprof.StopCPUProfile()
	return f.Close()
}

// tracedItem is a heap entry that remembers the edge that pushed it; ordered by dist only, so
// container/heap pops in the same order as with item.
type tracedItem struct {
	dist, weight int64
	node, from   int
}

type tracedHeap []tracedItem

func (h tracedHeap) Len() int           { return len(h) }
func (h tracedHeap) Less(i, j int) bool { return h[i].dist < h[j].dist }
func (h tracedHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }
func (h *tracedHeap) Push(x any)        { *h = append(*h, x.(tracedItem)) }
func (h *tracedHeap) Pop() any {
	old := *h
	last := old[len(old)-1]
	*h = old[:len(old)-1]
	return last
}

type exportEdge struct {
	To     int   `json:"to"`
	Weight int64 `json:"weight"`
}

type settledNode struct {
	Order  int          `json:"order"`
	Pop    int          `json:"pop"`
	Node   int          `json:"node"`
	Dist   int64        `json:"dist"`
	From   int          `json:"from"`
	Weight int64        `json:"weight"`
	Edges  []exportEdge `json:"edges"`
}

type stalePop struct {
	Pop         int   `json:"pop"`
	Node        int   `json:"node"`
	StaleDist   int64 `json:"stale_dist"`
	StaleFrom   int   `json:"stale_from"`
	StaleWeight int64 `json:"stale_weight"`
	FinalDist   int64 `json:"final_dist"`
	SettledAt   int   `json:"settled_at_order"`
}

type dijkstraExport struct {
	Heap           string        `json:"heap"`
	Size           int           `json:"size"`
	Checksum       string        `json:"checksum"`
	Settled        []settledNode `json:"settled"`
	FirstStalePop  *stalePop     `json:"first_stale_pop"`
	Pushes         int           `json:"pushes"`
	Pops           int           `json:"pops"`
	StalePops      int           `json:"stale_pops"`
	EntryBytesItem uintptr       `json:"entry_bytes_item"`
}

// dijkstraTrace is dijkstra (container/heap) that records the first `keep` settled nodes, the first
// stale pop and the push and pop counts. It is never timed.
func dijkstraTrace(n int, g graphInput, keep int) dijkstraExport {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	settledAt, settled := make([]int, n), 0
	ex := dijkstraExport{Heap: "container/heap", Size: n, Pushes: 1}
	h := &tracedHeap{{dist: 0, node: 0, from: -1}}
	for h.Len() > 0 {
		top := heap.Pop(h).(tracedItem)
		ex.Pops++
		if top.dist > dist[top.node] {
			ex.StalePops++
			if ex.FirstStalePop == nil {
				ex.FirstStalePop = &stalePop{
					Pop: ex.Pops, Node: top.node, StaleDist: top.dist, StaleFrom: top.from,
					StaleWeight: top.weight, FinalDist: dist[top.node], SettledAt: settledAt[top.node],
				}
			}
			continue
		}
		settled++
		settledAt[top.node] = settled
		if len(ex.Settled) < keep {
			s := settledNode{
				Order: settled, Pop: ex.Pops, Node: top.node, Dist: top.dist,
				From: top.from, Weight: top.weight,
			}
			for e := 4 * top.node; e < 4*top.node+4; e++ {
				s.Edges = append(s.Edges, exportEdge{To: g.to[e], Weight: g.weight[e]})
			}
			ex.Settled = append(ex.Settled, s)
		}
		for e := 4 * top.node; e < 4*top.node+4; e++ {
			v := g.to[e]
			nd := top.dist + g.weight[e]
			if nd < dist[v] {
				dist[v] = nd
				ex.Pushes++
				heap.Push(h, tracedItem{dist: nd, weight: g.weight[e], node: v, from: top.node})
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
	ex.Checksum = fmt.Sprintf("%d:%d:%d", reached, total, longest)
	return ex
}

// writeDijkstraExport writes dijkstraTrace as JSON (KERNELS_DIJKSTRA_EXPORT=<file>,
// KERNELS_DIJKSTRA_EXPORT_NODES, default 40).
func writeDijkstraExport(path string, n int, g graphInput, entryBytes uintptr) error {
	keep := 40
	if s := os.Getenv("KERNELS_DIJKSTRA_EXPORT_NODES"); s != "" {
		v, err := strconv.Atoi(s)
		if err != nil || v <= 0 {
			return fmt.Errorf("KERNELS_DIJKSTRA_EXPORT_NODES=%q: want a positive integer", s)
		}
		keep = v
	}
	ex := dijkstraTrace(n, g, keep)
	ex.EntryBytesItem = entryBytes
	data, err := json.MarshalIndent(ex, "", " ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, append(data, '\n'), 0o644)
}
