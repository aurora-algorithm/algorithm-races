// Race: skiplist (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
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
