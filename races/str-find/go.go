// Race: str-find (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
	"strings"
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
