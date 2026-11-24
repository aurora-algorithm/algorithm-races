package main

// Classic algorithm kernels (../SPEC.md section 12, group algo).

import (
	"cmp"
	"container/heap"
	"container/list"
	"fmt"
	"maps"
	"math"
	"os"
	"runtime"
	"slices"
	"strings"
	"unsafe"
)

const (
	knapsackItems = 1000
	patternLen    = 8
	acgt          = "acgt"
)

// englishWords: the 64 most common English words (../SPEC.md section 12, kmp-en).
var englishWords = strings.Fields("the be to of and a in that have i it for not on with he as you do at this but his by " +
	"from they we say her she or an will my one all would there their what so up out if about who get which go me " +
	"when make can like time no just him know take people into year your")

func init() {
	maps.Copy(kernels, map[string]kernel{
		"bfs-grid":      bfsGridKernel,
		"kruskal":       kruskalKernel,
		"bst":           bstKernel,
		"trie":          trieKernel,
		"lru":           lruKernel,
		"edit-distance": editDistanceKernel,
		"knapsack":      knapsackKernel,
		"lis":           lisKernel,
		"kmp":           textKernel(kmp),
		"str-find":      textKernel(strFind),
		"kmp-en":        englishKernel(kmp),
		"str-find-en":   englishKernel(strFind),
		"fib":           sizeKernel(fib, func(v int) string { return fmt.Sprint(v) }),
		"nqueens":       sizeKernel(nqueens, func(v int) string { return fmt.Sprint(v) }),
		"binary-search": binarySearchKernel,
	})
}

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

type graphInput struct {
	to     []int
	weight []int64
}

// graph gives every node u four edges at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
func graph(n int) graphInput {
	rng := newRNG()
	g := graphInput{to: make([]int, 4*n), weight: make([]int64, 4*n)}
	for u := range n {
		g.to[4*u] = (u + 1) % n
		g.weight[4*u] = 1 + int64(rng.Next()%100)
		for e := 4*u + 1; e < 4*u+4; e++ {
			g.to[e] = int(rng.Next() % uint32(n))
			g.weight[e] = 1 + int64(rng.Next()%100)
		}
	}
	return g
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

// binHeap is a typed binary min-heap: no interface boxing on push, no type assertion on pop.
type binHeap[T less[T]] struct{ items []T }

func (h *binHeap[T]) push(x T) {
	h.items = append(h.items, x)
	i := len(h.items) - 1
	for i > 0 {
		p := (i - 1) / 2
		if !x.less(h.items[p]) {
			break
		}
		h.items[i] = h.items[p]
		i = p
	}
	h.items[i] = x
}

func (h *binHeap[T]) pop() T {
	top := h.items[0]
	last := h.items[len(h.items)-1]
	h.items = h.items[:len(h.items)-1]
	m := len(h.items)
	if m == 0 {
		return top
	}
	i := 0
	for {
		c := 2*i + 1
		if c >= m {
			break
		}
		if c+1 < m && h.items[c+1].less(h.items[c]) {
			c++
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

// dijkstraTyped is dijkstra with binHeap in place of container/heap (runtime go-typedheap).
func dijkstraTyped(n int, g graphInput) string {
	dist := make([]int64, n)
	for i := range dist {
		dist[i] = math.MaxInt64
	}
	dist[0] = 0
	h := &binHeap[item]{items: []item{{dist: 0, node: 0}}}
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

// dijkstraKernel reports heap allocations per run (runtime.MemStats, read outside the timed region) and the heap
// entry size in extra. KERNELS_DIJKSTRA_HEAP=typed switches to binHeap, typed4 to quadHeap, typedswap to swapHeap, container32 and typed32
// to C#'s 32-bit node ids (dijkstra32.go). After the timed repeats, KERNELS_DIJKSTRA_COUNTS=1 adds push and pop
// counts, KERNELS_DIJKSTRA_PROFILE writes a CPU profile and KERNELS_DIJKSTRA_EXPORT the trace of the first settled
// nodes (dijkstra_extra.go).
func dijkstraKernel(c config) (string, []float64, map[string]any, error) {
	switch os.Getenv("KERNELS_DIJKSTRA_HEAP") {
	case "container32":
		return dijkstra32Kernel(c, false)
	case "typed32":
		return dijkstra32Kernel(c, true)
	}
	g := graph(c.size)
	search, heapName := dijkstra, "container/heap"
	switch os.Getenv("KERNELS_DIJKSTRA_HEAP") {
	case "typed":
		search, heapName = dijkstraTyped, "typed binHeap[item]"
	case "typed4":
		search, heapName = dijkstraTyped4, "typed quadHeap[item]"
	case "typedswap":
		search, heapName = dijkstraSwap, "typed swapHeap[item]"
	}
	var before, after runtime.MemStats
	runtime.ReadMemStats(&before)
	out, times := measure(c.repeats, same(g), func(g graphInput) string { return search(c.size, g) })
	runtime.ReadMemStats(&after)
	runs := uint64(c.repeats)
	extra := map[string]any{
		"heap":              heapName,
		"mallocs_per_run":   (after.Mallocs - before.Mallocs) / runs,
		"alloc_mb_per_run":  math.Round(float64(after.TotalAlloc-before.TotalAlloc)/float64(runs)/1e4) / 100,
		"gc_cycles_per_run": float64(after.NumGC-before.NumGC) / float64(runs),
		"entry_bytes":       unsafe.Sizeof(item{}),
	}
	if os.Getenv("KERNELS_DIJKSTRA_COUNTS") == "1" {
		pushes, pops, stale, sum := dijkstraCounts(c.size, g, heapName)
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
	if path := os.Getenv("KERNELS_DIJKSTRA_EXPORT"); path != "" {
		if err := writeDijkstraExport(path, c.size, g, unsafe.Sizeof(item{})); err != nil {
			return "", nil, nil, fmt.Errorf("export: %w", err)
		}
	}
	return out, times, extra, nil
}

type edge struct {
	w    uint32
	u, v int
}

func edgeList(n int) []edge {
	rng := newRNG()
	edges := make([]edge, 4*n)
	for i := range edges {
		u := int(rng.Next() % uint32(n))
		v := int(rng.Next() % uint32(n))
		edges[i] = edge{w: rng.Next() % 1000000, u: u, v: v}
	}
	return edges
}

func find(parent []int, x int) int {
	root := x
	for parent[root] != root {
		root = parent[root]
	}
	for parent[x] != root {
		parent[x], x = root, parent[x]
	}
	return root
}

func kruskal(n int, edges []edge) string {
	slices.SortFunc(edges, func(a, b edge) int { return cmp.Compare(a.w, b.w) })
	parent := make([]int, n)
	for i := range parent {
		parent[i] = i
	}
	rank := make([]uint8, n)
	count, total := 0, uint64(0)
	for _, e := range edges {
		ru, rv := find(parent, e.u), find(parent, e.v)
		if ru == rv {
			continue
		}
		if rank[ru] < rank[rv] {
			ru, rv = rv, ru
		}
		parent[rv] = ru
		if rank[ru] == rank[rv] {
			rank[ru]++
		}
		count++
		total += uint64(e.w)
	}
	return fmt.Sprintf("%d:%d", count, total)
}

func kruskalKernel(c config) (string, []float64, error) {
	base := edgeList(c.size)
	run := func(e []edge) string { return kruskal(c.size, e) }
	out, times := measure(c.repeats, func() []edge { return slices.Clone(base) }, run)
	return out, times, nil
}

type node struct {
	key         int64
	count       int
	left, right *node
}

func bst(in hashmapInput) string {
	var root *node
	size := 0
	for _, k := range in.inserts {
		link := &root
		for *link != nil && (*link).key != k {
			if k < (*link).key {
				link = &(*link).left
			} else {
				link = &(*link).right
			}
		}
		if *link == nil {
			*link = &node{key: k, count: 1}
			size++
		} else {
			(*link).count++
		}
	}
	hits := 0
	for _, k := range in.lookups {
		n := root
		for n != nil && n.key != k {
			if k < n.key {
				n = n.left
			} else {
				n = n.right
			}
		}
		if n != nil {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d:%d", size, hits, h31(inOrder(root, size)))
}

func inOrder(root *node, size int) []int64 {
	keys := make([]int64, 0, size)
	var stack []*node
	n := root
	for n != nil || len(stack) > 0 {
		for n != nil {
			stack = append(stack, n)
			n = n.left
		}
		n = stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		keys = append(keys, n.key)
		n = n.right
	}
	return keys
}

func bstKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(newHashmapInput(c.size)), bst)
	return out, times, nil
}

type trieInput struct {
	inserted, prefixes []string
}

func word(rng *XorShift32, minLen, spread uint32) string {
	b := make([]byte, minLen+rng.Next()%spread)
	for i := range b {
		b[i] = byte('a' + rng.Next()%26)
	}
	return string(b)
}

func words(n int) trieInput {
	rng := newRNG()
	in := trieInput{inserted: make([]string, n), prefixes: make([]string, n)}
	for i := range in.inserted {
		in.inserted[i] = word(rng, 3, 8)
	}
	for i := range in.prefixes {
		in.prefixes[i] = word(rng, 1, 4)
	}
	return in
}

type trieNode struct {
	count    int
	children [26]*trieNode
}

func trie(in trieInput) string {
	root := &trieNode{}
	nodes := 0
	for _, w := range in.inserted {
		n := root
		for i := range len(w) {
			ch := w[i] - 'a'
			if n.children[ch] == nil {
				n.children[ch] = &trieNode{}
				nodes++
			}
			n = n.children[ch]
			n.count++
		}
	}
	total := 0
	for _, p := range in.prefixes {
		n := root
		for i := 0; i < len(p) && n != nil; i++ {
			n = n.children[p[i]-'a']
		}
		if n != nil {
			total += n.count
		}
	}
	return fmt.Sprintf("%d:%d", nodes, total)
}

func trieKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(words(c.size)), trie)
	return out, times, nil
}

type lruInput struct {
	capacity int
	keys     []int
}

func lruKeys(n int) lruInput {
	capacity := max(1, n/100)
	rng := newRNG()
	keys := make([]int, n)
	for i := range keys {
		keys[i] = int(rng.Next() % uint32(2*capacity))
	}
	return lruInput{capacity: capacity, keys: keys}
}

// lru keeps the most recent key at the front of a container/list; the map finds a key's element.
func lru(in lruInput) string {
	order := list.New()
	elems := make(map[int]*list.Element, in.capacity)
	hits := 0
	for _, k := range in.keys {
		if e, ok := elems[k]; ok {
			order.MoveToFront(e)
			hits++
			continue
		}
		if order.Len() == in.capacity {
			oldest := order.Back()
			delete(elems, oldest.Value.(int))
			order.Remove(oldest)
		}
		elems[k] = order.PushFront(k)
	}
	keys := make([]int64, 0, order.Len())
	for e := order.Back(); e != nil; e = e.Prev() {
		keys = append(keys, int64(e.Value.(int)))
	}
	return fmt.Sprintf("%d:%d:%d", hits, len(in.keys)-hits, h31(keys))
}

func lruKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(lruKeys(c.size)), lru)
	return out, times, nil
}

func dna(length, count int) []string {
	rng := newRNG()
	out := make([]string, count)
	for i := range out {
		b := make([]byte, length)
		for j := range b {
			b[j] = acgt[rng.Next()%4]
		}
		out[i] = string(b)
	}
	return out
}

func editDistance(ab []string) string {
	a, b := ab[0], ab[1]
	prev := make([]int, len(b)+1)
	cur := make([]int, len(b)+1)
	for j := range prev {
		prev[j] = j
	}
	for i := range len(a) {
		cur[0] = i + 1
		for j := range len(b) {
			cost := 1
			if a[i] == b[j] {
				cost = 0
			}
			cur[j+1] = min(prev[j+1]+1, cur[j]+1, prev[j]+cost)
		}
		prev, cur = cur, prev
	}
	return fmt.Sprint(prev[len(b)])
}

func editDistanceKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(dna(c.size, 2)), editDistance)
	return out, times, nil
}

type thing struct {
	weight int
	value  int64
}

func items() []thing {
	rng := newRNG()
	out := make([]thing, knapsackItems)
	for i := range out {
		weight := 1 + int(rng.Next()%1000)
		out[i] = thing{weight: weight, value: 1 + int64(rng.Next()%1000)}
	}
	return out
}

func knapsack(capacity int, things []thing) string {
	best := make([]int64, capacity+1)
	for _, t := range things {
		for c := capacity; c >= t.weight; c-- {
			if best[c-t.weight]+t.value > best[c] {
				best[c] = best[c-t.weight] + t.value
			}
		}
	}
	return fmt.Sprintf("%d:%d", best[capacity], h31(best))
}

func knapsackKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(items()), func(t []thing) string { return knapsack(c.size, t) })
	return out, times, nil
}

func lis(values []int64) string {
	var tails []int64
	for _, x := range values {
		i, _ := slices.BinarySearch(tails, x)
		if i == len(tails) {
			tails = append(tails, x)
		} else {
			tails[i] = x
		}
	}
	return fmt.Sprintf("%d:%d", len(tails), h31(tails))
}

func lisKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(ints(c.size)), lis)
	return out, times, nil
}

type textInput struct {
	text, pattern string
}

type matches struct {
	count, first, last int
}

func (m *matches) add(pos int) {
	if m.count == 0 {
		m.first = pos
	}
	m.last = pos
	m.count++
}

func (m matches) String() string {
	if m.count == 0 {
		return "0:-1:-1"
	}
	return fmt.Sprintf("%d:%d:%d", m.count, m.first, m.last)
}

func kmp(in textInput) matches {
	t, p := in.text, in.pattern
	m := len(p)
	fail := make([]int, m)
	k := 0
	for i := 1; i < m; i++ {
		for k > 0 && p[i] != p[k] {
			k = fail[k-1]
		}
		if p[i] == p[k] {
			k++
		}
		fail[i] = k
	}
	var found matches
	k = 0
	for i := range len(t) {
		for k > 0 && t[i] != p[k] {
			k = fail[k-1]
		}
		if t[i] == p[k] {
			k++
		}
		if k == m {
			found.add(i + 1 - m)
			k = fail[k-1]
		}
	}
	return found
}

func strFind(in textInput) matches {
	var found matches
	start := 0
	for {
		offset := strings.Index(in.text[start:], in.pattern)
		if offset < 0 {
			return found
		}
		found.add(start + offset)
		start += offset + 1
	}
}

func textKernel(search func(textInput) matches) kernel {
	return func(c config) (string, []float64, error) {
		t := dna(c.size, 1)[0]
		in := textInput{text: t, pattern: t[c.size/2 : c.size/2+patternLen]}
		out, times := measure(c.repeats, same(in), search)
		return out.String(), times, nil
	}
}

// english gives n bytes of words separated by " ", ", " or ". " (../SPEC.md section 12, kmp-en).
func english(n int) string {
	rng := newRNG()
	var b strings.Builder
	b.Grow(n + 16)
	for b.Len() < n {
		x := rng.Next() % 64
		y := rng.Next() % 64
		b.WriteString(englishWords[min(x, y)])
		switch rng.Next() % 16 {
		case 0:
			b.WriteString(". ")
		case 1:
			b.WriteString(", ")
		default:
			b.WriteString(" ")
		}
	}
	return b.String()[:n]
}

func englishKernel(search func(textInput) matches) kernel {
	return func(c config) (string, []float64, error) {
		t := english(c.size)
		in := textInput{text: t, pattern: t[c.size/2 : c.size/2+patternLen]}
		out, times := measure(c.repeats, same(in), search)
		return out.String(), times, nil
	}
}

func fib(n int) int {
	if n < 2 {
		return n
	}
	return fib(n-1) + fib(n-2)
}

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

type searchInput struct {
	a, queries []int64
}

func sortedArray(n int) searchInput {
	rng := newRNG()
	in := searchInput{a: make([]int64, n), queries: make([]int64, n)}
	v := int64(0)
	for i := range in.a {
		v += 1 + int64(rng.Next()%4)
		in.a[i] = v
	}
	for i := range in.queries {
		in.queries[i] = int64(rng.Next()) % (v + 1)
	}
	return in
}

func binarySearch(in searchInput) string {
	n := len(in.a)
	hits, total := 0, 0
	for _, q := range in.queries {
		lo, hi := 0, n
		for lo < hi {
			mid := (lo + hi) / 2
			if in.a[mid] < q {
				lo = mid + 1
			} else {
				hi = mid
			}
		}
		if lo < n && in.a[lo] == q {
			hits++
		}
		total += lo
	}
	return fmt.Sprintf("%d:%d", hits, total)
}

func binarySearchKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(sortedArray(c.size)), binarySearch)
	return out, times, nil
}
