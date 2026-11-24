// Race: huffman (go)
// Extracted from suite/go/puzzle_text.go — build and run via suite/run.py

package main
import (
	"bytes"
	"container/heap"
)

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
