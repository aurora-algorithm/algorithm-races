// Race: palindrome (go)
// Extracted from suite/go/puzzle_text.go — build and run via suite/run.py

package main
import (
	"fmt"
)

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
