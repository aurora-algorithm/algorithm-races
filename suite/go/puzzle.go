package main

// The "puzzle" kernels (kernels/SPEC.md section 13): one clever algorithm each, the same steps in every language.

import (
	"container/heap"
	"fmt"
	"math"
	"math/big"
	"math/bits"
	"slices"
	"strconv"
	"strings"
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

var windows = []int{10, 100, 1000, 10000, 100000}

// puzzleKernel builds the input from the size once; no puzzle kernel changes its input, so every repeat reuses it.
func puzzleKernel[In, Out any](prepare func(n int) In, run func(In) Out, check func(Out) string) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(prepare(c.size)), run)
		return check(out), times, nil
	}
}

var puzzleKernels = map[string]kernel{
	"sudoku":       puzzleKernel(sudokuPuzzles, sudoku, identity),
	"bloom":        puzzleKernel(bloomInput, bloom, identity),
	"hyperloglog":  sizeKernel(hyperloglog, identity),
	"skiplist":     puzzleKernel(newHashmapInput, skiplist, skiplistChecksum),
	"cycle":        puzzleKernel(cycleStarts, cycle, identity),
	"palindrome":   puzzleKernel(binaryText, palindrome, identity),
	"bwt":          puzzleKernel(dnaText, bwt, bwtChecksum),
	"aho-corasick": puzzleKernel(newAhoInput, ahoCorasick, identity),
	"pi-digits":    sizeKernel(piDigits, piChecksum),
	"life":         puzzleKernel(newLifeInput, life, lifeChecksum),
	"huffman":      puzzleKernel(skewedText, huffman, huffmanChecksum),
	"window-max":   puzzleKernel(ints, windowMax, identity),
	"rain-water":   puzzleKernel(terrain, rainWater, identity),
	"collatz":      sizeKernel(collatz, identity),
}

// fmix32 is MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
func fmix32(h uint32) uint32 {
	h ^= h >> 16
	h *= 0x85ebca6b
	h ^= h >> 13
	h *= 0xc2b2ae35
	h ^= h >> 16
	return h
}

func shuffle[T any](rng *XorShift32, items []T) {
	for i := len(items) - 1; i > 0; i-- {
		j := int(rng.Next() % uint32(i+1))
		items[i], items[j] = items[j], items[i]
	}
}

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

// sudoku: Knuth's Algorithm X with dancing links

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

// bloom: k bit positions per key from two hashes

func bloomInput(n int) hashmapInput {
	values := ints(2 * n)
	return hashmapInput{inserts: values[:n], lookups: values[n:]}
}

func bloom(in hashmapInput) string {
	m := uint32(bloomBitsPerKey * len(in.inserts))
	filter := make([]byte, (m+7)/8)
	for _, key := range in.inserts {
		h1 := fmix32(uint32(key))
		h2 := fmix32(uint32(key)^golden) | 1
		for i := range uint32(bloomHashes) {
			b := (h1 + i*h2) % m
			filter[b>>3] |= 1 << (b & 7)
		}
	}
	falsePositives := 0
	for _, key := range in.lookups {
		h1 := fmix32(uint32(key))
		h2 := fmix32(uint32(key)^golden) | 1
		found := true
		for i := range uint32(bloomHashes) {
			b := (h1 + i*h2) % m
			if filter[b>>3]&(1<<(b&7)) == 0 {
				found = false
				break
			}
		}
		if found {
			falsePositives++
		}
	}
	setBits := 0
	for _, b := range filter {
		setBits += bits.OnesCount8(b)
	}
	return fmt.Sprintf("%d:%d:%d", len(in.inserts), setBits, falsePositives)
}

// hyperloglog: the longest run of leading zeros estimates how many distinct items went by

func hyperloglog(n int) string {
	rng := newRNG()
	universe := uint32(max(n/4, 1))
	registers := make([]byte, hllM)
	for range n {
		h := fmix32(rng.Next() % universe)
		w := h << hllP
		rank := byte(32 - hllP + 1)
		if w != 0 {
			rank = byte(bits.LeadingZeros32(w) + 1)
		}
		idx := h >> (32 - hllP)
		if rank > registers[idx] {
			registers[idx] = rank
		}
	}
	z := 0.0
	zeros := 0
	values := make([]int64, hllM)
	for i, r := range registers {
		z += math.Ldexp(1, -int(r))
		if r == 0 {
			zeros++
		}
		values[i] = int64(r)
	}
	m := float64(hllM)
	estimate := 0.7213 / (1.0 + 1.079/m) * m * m / z
	if estimate <= 2.5*m && zeros > 0 {
		estimate = m * math.Log(m/float64(zeros))
	}
	return fmt.Sprintf("%d:%d:%d", n, int64(math.Floor(estimate+0.5)), h31(values))
}

// skiplist: towers of random height turn a linked list into a search tree

type skipNode struct {
	key  int64
	next []*skipNode
}

type skiplistResult struct {
	head                *skipNode
	n, hits, heightsSum int
}

func skiplist(in hashmapInput) skiplistResult {
	rng := &XorShift32{s: skipLevelSeed}
	head := &skipNode{key: -1, next: make([]*skipNode, skipMaxLevel)}
	level := 1
	update := make([]*skipNode, skipMaxLevel)
	for i := range update {
		update[i] = head
	}
	heightsSum := 0
	for _, key := range in.inserts {
		x := head
		for i := level - 1; i >= 0; i-- {
			for x.next[i] != nil && x.next[i].key < key {
				x = x.next[i]
			}
			update[i] = x
		}
		if nxt := x.next[0]; nxt != nil && nxt.key == key {
			continue
		}
		height := 1
		for height < skipMaxLevel && rng.Next()&1 != 0 {
			height++
		}
		for i := level; i < height; i++ {
			update[i] = head
		}
		level = max(level, height)
		node := &skipNode{key: key, next: make([]*skipNode, height)}
		for i := range height {
			node.next[i] = update[i].next[i]
			update[i].next[i] = node
		}
		heightsSum += height
	}
	hits := 0
	for _, key := range in.lookups {
		x := head
		for i := level - 1; i >= 0; i-- {
			for x.next[i] != nil && x.next[i].key < key {
				x = x.next[i]
			}
		}
		if nxt := x.next[0]; nxt != nil && nxt.key == key {
			hits++
		}
	}
	return skiplistResult{head: head, n: len(in.inserts), hits: hits, heightsSum: heightsSum}
}

func skiplistChecksum(r skiplistResult) string {
	var ordered []int64
	for x := r.head.next[0]; x != nil; x = x.next[0] {
		ordered = append(ordered, x.key)
	}
	return fmt.Sprintf("%d:%d:%d:%d", r.n, r.hits, r.heightsSum, h31(ordered))
}

// cycle: Floyd's tortoise and hare on x -> x*x + c mod M

func cycleStarts(n int) []int64 {
	rng := newRNG()
	starts := make([]int64, n)
	for i := range starts {
		starts[i] = int64(rng.Next() % cycleMod)
	}
	return starts
}

func cycleStep(x int64) int64 {
	return (x*x + cycleC) % cycleMod
}

func cycle(starts []int64) string {
	totalMu, totalLam, longest := 0, 0, 0
	for _, x0 := range starts {
		tortoise := cycleStep(x0)
		hare := cycleStep(tortoise)
		for tortoise != hare {
			tortoise = cycleStep(tortoise)
			hare = cycleStep(cycleStep(hare))
		}
		mu := 0
		tortoise = x0
		for tortoise != hare {
			tortoise = cycleStep(tortoise)
			hare = cycleStep(hare)
			mu++
		}
		lam := 1
		hare = cycleStep(tortoise)
		for tortoise != hare {
			hare = cycleStep(hare)
			lam++
		}
		totalMu += mu
		totalLam += lam
		longest = max(longest, mu+lam)
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(starts), totalMu, totalLam, longest)
}

// pi-digits: the Chudnovsky series summed as one exact fraction by binary splitting

const (
	chudA         = 13591409
	chudB         = 545140134
	digitsPerTerm = 14
	guardDigits   = 10
)

// chudC324 is 640320^3 / 24.
var chudC324 = big.NewInt(10939058860032000)

// binarySplit returns P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
func binarySplit(a, b int64) (p, q, t *big.Int) {
	if b-a == 1 {
		if a == 0 {
			p, q = big.NewInt(1), big.NewInt(1)
		} else {
			p = big.NewInt((6*a - 5) * (2*a - 1) * (6*a - 1))
			q = new(big.Int).Mul(big.NewInt(a*a*a), chudC324)
		}
		t = new(big.Int).Mul(p, big.NewInt(chudA+chudB*a))
		if a%2 == 1 {
			t.Neg(t)
		}
		return p, q, t
	}
	m := (a + b) / 2
	p1, q1, t1 := binarySplit(a, m)
	p2, q2, t2 := binarySplit(m, b)
	t = new(big.Int).Mul(t1, q2)
	t.Add(t, new(big.Int).Mul(p1, t2))
	return new(big.Int).Mul(p1, p2), new(big.Int).Mul(q1, q2), t
}

func powerOfTen(k int) *big.Int {
	return new(big.Int).Exp(big.NewInt(10), big.NewInt(int64(k)), nil)
}

// isqrt is the floor square root of n > 0 by precision doubling (CPython's math.isqrt), written out as in the others.
func isqrt(n *big.Int) *big.Int {
	c := (n.BitLen() - 1) / 2
	a := big.NewInt(1)
	d := 0
	for s := bits.Len(uint(c)) - 1; s >= 0; s-- {
		e := d
		d = c >> s
		q := new(big.Int).Rsh(n, uint(2*c-e-d+1))
		q.Quo(q, a)
		a.Lsh(a, uint(d-e-1)).Add(a, q)
	}
	if new(big.Int).Mul(a, a).Cmp(n) > 0 {
		a.Sub(a, big.NewInt(1))
	}
	return a
}

func piDigits(n int) string {
	terms := int64(n/digitsPerTerm + 2)
	_, q, t := binarySplit(0, terms)
	one := powerOfTen(n + guardDigits)
	sqrtC := new(big.Int).Mul(one, one)
	sqrtC = isqrt(sqrtC.Mul(sqrtC, big.NewInt(10005)))
	pi := new(big.Int).Mul(big.NewInt(426880), sqrtC)
	pi.Mul(pi, q)
	pi.Quo(pi, t)
	pi.Quo(pi, powerOfTen(guardDigits))
	return pi.String()
}

func piChecksum(digits string) string {
	sum := 0
	for _, ch := range digits {
		sum += int(ch - '0')
	}
	return fmt.Sprintf("%d:%s:%d", len(digits)-1, digits[len(digits)-50:], sum)
}

// life: Conway's Game of Life on a torus

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

// window-max: a monotonic deque keeps the maximum of every window in O(1) per step

func windowMax(values []int64) string {
	n := len(values)
	dq := make([]int, n)
	hashes := make([]string, 0, len(windows))
	for _, w := range windows {
		head, tail := 0, 0
		var h uint32
		for i, v := range values {
			for tail > head && values[dq[tail-1]] <= v {
				tail--
			}
			dq[tail] = i
			tail++
			if dq[head] <= i-w {
				head++
			}
			if i >= w-1 {
				h = h*31 + uint32(values[dq[head]])
			}
		}
		hashes = append(hashes, formatHash(h))
	}
	return strconv.Itoa(n) + ":" + strings.Join(hashes, ":")
}

// rain-water: flood a 2D terrain from its walls inward with a min-heap

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

// collatz: remember every chain length below the start so each walk stops early

func collatz(n int) string {
	cache := make([]int32, n)
	cache[1] = 1
	best, bestLen, total := 1, int32(1), int64(1)
	for start := 2; start < n; start++ {
		x := int64(start)
		steps := int32(0)
		for x >= int64(start) {
			if x%2 == 0 {
				x /= 2
			} else {
				x = 3*x + 1
			}
			steps++
		}
		length := steps + cache[x]
		cache[start] = length
		total += int64(length)
		if length > bestLen {
			best, bestLen = start, length
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d", n, best, bestLen, total)
}
