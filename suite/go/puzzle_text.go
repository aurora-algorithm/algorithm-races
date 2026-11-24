package main

// The text kernels of the "puzzle" group (kernels/SPEC.md section 13): palindrome, bwt, aho-corasick, huffman.

import (
	"bytes"
	"cmp"
	"container/heap"
	"fmt"
	"slices"
)

// palindrome: Manacher's mirror trick finds every palindrome in linear time

// binaryText spells `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
func binaryText(n int) []byte {
	rng := newRNG()
	out := make([]byte, n)
	for i := 0; i < n; i += 32 {
		v := rng.Next()
		for j := i; j < min(i+32, n); j++ {
			out[j] = 'a' + byte(v&1)
			v >>= 1
		}
	}
	return out
}

func palindrome(text []byte) string {
	t := make([]byte, 2*len(text)+3)
	for i := range t {
		t[i] = '#'
	}
	t[0], t[len(t)-1] = '^', '$'
	for i, ch := range text {
		t[2+2*i] = ch
	}
	p := make([]int, len(t))
	center, right := 0, 0
	for i := 1; i < len(t)-1; i++ {
		if i < right {
			p[i] = min(right-i, p[2*center-i])
		}
		for t[i+1+p[i]] == t[i-1-p[i]] {
			p[i]++
		}
		if i+p[i] > right {
			center, right = i, i+p[i]
		}
	}
	longest, best, count := 0, 0, 0
	for i, radius := range p {
		count += (radius + 1) / 2
		if radius > longest {
			longest, best = radius, i
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(text), longest, (best-1-longest)/2, count)
}

// bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

// dnaText spells `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
func dnaText(n int) []byte {
	return dnaFrom(newRNG(), n)
}

func dnaFrom(rng *XorShift32, n int) []byte {
	out := make([]byte, n)
	for i := 0; i < n; i += 16 {
		v := rng.Next()
		for j := i; j < min(i+16, n); j++ {
			out[j] = dnaLetters[v&3]
			v >>= 2
		}
	}
	return out
}

type bwtResult struct {
	n, rounds, primary int
	out                []byte
}

func bwt(dna []byte) bwtResult {
	text := append(slices.Clone(dna), '$')
	n := len(text)
	rank := make([]int, n)
	sa := make([]int, n)
	for i, ch := range text {
		rank[i] = int(ch)
		sa[i] = i
	}
	second := make([]int, n)
	k, rounds := 1, 0
	for {
		rounds++
		for i := range second {
			second[i] = -1
			if i+k < n {
				second[i] = rank[i+k]
			}
		}
		slices.SortFunc(sa, func(a, b int) int {
			return cmp.Or(cmp.Compare(rank[a], rank[b]), cmp.Compare(second[a], second[b]))
		})
		tmp := make([]int, n)
		for j := 1; j < n; j++ {
			a, b := sa[j-1], sa[j]
			tmp[b] = tmp[a]
			if rank[a] != rank[b] || second[a] != second[b] {
				tmp[b]++
			}
		}
		rank = tmp
		if rank[sa[n-1]] == n-1 {
			break
		}
		k *= 2
	}
	out := make([]byte, n)
	primary := 0
	for j, i := range sa {
		if i == 0 {
			primary = j
			out[j] = text[n-1]
		} else {
			out[j] = text[i-1]
		}
	}
	return bwtResult{n: n, rounds: rounds, primary: primary, out: out}
}

func bwtChecksum(r bwtResult) string {
	return fmt.Sprintf("%d:%d:%d:%d", r.n, r.rounds, r.primary, fnv1a(r.out))
}

// aho-corasick: a trie with failure links matches thousands of patterns in one pass

type ahoInput struct {
	patterns [][]byte
	text     []byte
}

func newAhoInput(n int) ahoInput {
	rng := newRNG()
	patterns := make([][]byte, ahoPatterns)
	for p := range patterns {
		length := 6 + rng.Next()%7
		pattern := make([]byte, length)
		for i := range pattern {
			pattern[i] = dnaLetters[rng.Next()%4]
		}
		patterns[p] = pattern
	}
	block := dnaFrom(rng, oneMiB)
	return ahoInput{patterns: patterns, text: bytes.Repeat(block, n/oneMiB)}
}

func ahoCorasick(in ahoInput) string {
	var code [256]int
	for i := range len(dnaLetters) {
		code[dnaLetters[i]] = i
	}
	edges := []int{-1, -1, -1, -1}
	out := []int{0}
	for _, pattern := range in.patterns {
		s := 0
		for _, ch := range pattern {
			edge := s*4 + code[ch]
			if edges[edge] == -1 {
				edges[edge] = len(out)
				edges = append(edges, -1, -1, -1, -1)
				out = append(out, 0)
			}
			s = edges[edge]
		}
		out[s]++
	}
	fail := make([]int, len(out))
	var queue []int
	for c := range 4 {
		if edges[c] == -1 {
			edges[c] = 0
		} else {
			queue = append(queue, edges[c])
		}
	}
	for head := 0; head < len(queue); head++ {
		s := queue[head]
		out[s] += out[fail[s]]
		for c := range 4 {
			t := edges[s*4+c]
			if t == -1 {
				edges[s*4+c] = edges[fail[s]*4+c]
			} else {
				fail[t] = edges[fail[s]*4+c]
				queue = append(queue, t)
			}
		}
	}
	s, matches, hits := 0, 0, 0
	for _, ch := range in.text {
		s = edges[s*4+code[ch]]
		if found := out[s]; found > 0 {
			matches += found
			hits++
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d", len(in.patterns), len(out), matches, hits)
}

// huffman: merge the two rarest symbols until one tree is left

// skewedText is a 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
func skewedText(n int) []byte {
	rng := newRNG()
	block := make([]byte, oneMiB)
	for i := range block {
		a := rng.Next() % 26
		b := rng.Next() % 26
		block[i] = byte(97 + a*b/26)
	}
	return bytes.Repeat(block, n/oneMiB)
}

type huffmanResult struct {
	n, bits int
	encoded []byte
	ok      bool
}

func huffman(text []byte) huffmanResult {
	var freq [256]int64
	for _, b := range text {
		freq[b]++
	}
	var queue packedHeap
	for sym, f := range freq {
		if f > 0 {
			queue = append(queue, f*1024+int64(sym))
		}
	}
	heap.Init(&queue)
	var left, right [512]int
	nextID := 256
	for queue.Len() > 1 {
		a := heap.Pop(&queue).(int64)
		b := heap.Pop(&queue).(int64)
		left[nextID], right[nextID] = int(a%1024), int(b%1024)
		heap.Push(&queue, (a/1024+b/1024)*1024+int64(nextID))
		nextID++
	}
	root := int(queue[0] % 1024)

	type entry struct{ node, code, length int }
	var codes, lengths [256]int
	stack := []entry{{root, 0, 0}}
	for len(stack) > 0 {
		e := stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		if e.node < 256 {
			codes[e.node], lengths[e.node] = e.code, e.length
		} else {
			stack = append(stack, entry{left[e.node], e.code << 1, e.length + 1})
			stack = append(stack, entry{right[e.node], e.code<<1 | 1, e.length + 1})
		}
	}

	var encoded []byte
	acc, nbits, total := 0, 0, 0
	for _, b := range text {
		acc = acc<<lengths[b] | codes[b]
		nbits += lengths[b]
		total += lengths[b]
		for nbits >= 8 {
			nbits -= 8
			encoded = append(encoded, byte(acc>>nbits))
		}
		acc &= 1<<nbits - 1
	}
	if nbits > 0 {
		encoded = append(encoded, byte(acc<<(8-nbits)))
	}

	decoded := make([]byte, len(text))
	node, k := root, 0
decode:
	for _, b := range encoded {
		for shift := 7; shift >= 0; shift-- {
			if b>>shift&1 == 1 {
				node = right[node]
			} else {
				node = left[node]
			}
			if node < 256 {
				decoded[k] = byte(node)
				k++
				node = root
				if k == len(text) {
					break decode
				}
			}
		}
	}
	return huffmanResult{n: len(text), bits: total, encoded: encoded, ok: bytes.Equal(decoded, text)}
}

func huffmanChecksum(r huffmanResult) string {
	return fmt.Sprintf("%d:%d:%d:%d", r.n, r.bits, fnv1a(r.encoded), b2i(r.ok))
}
