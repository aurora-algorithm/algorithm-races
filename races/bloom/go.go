// Race: bloom (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
	"math/bits"
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

// fmix32 is MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
func fmix32(h uint32) uint32 {
	h ^= h >> 16
	h *= 0x85ebca6b
	h ^= h >> 13
	h *= 0xc2b2ae35
	h ^= h >> 16
	return h
}

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
