// Race: kmp (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

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
