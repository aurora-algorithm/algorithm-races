// Race: wordcount (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"bufio"
	"cmp"
	"fmt"
	"os"
	"path/filepath"
)

type workKernel func(c config) (checksum string, timesMs []float64, extra map[string]any, err error)

func fileKernel[Out any](pattern string, run func(path string) (Out, error), check func(Out) string) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		path := filepath.Join(c.data, fmt.Sprintf(pattern, c.size))
		var runErr error
		out, times := measure(c.repeats, same(path), func(p string) Out {
			r, err := run(p)
			runErr = cmp.Or(runErr, err)
			return r
		})
		if runErr != nil {
			return "", nil, nil, runErr
		}
		return check(out), times, nil, nil
	}
}

func wordcount(path string) (map[string]int, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	counts := map[string]int{}
	scanner := bufio.NewScanner(f)
	scanner.Split(bufio.ScanWords)
	for scanner.Scan() {
		counts[scanner.Text()]++
	}
	return counts, scanner.Err()
}

func wordcountChecksum(counts map[string]int) string {
	total, top, topWord := 0, 0, ""
	for w, n := range counts {
		total += n
		if n > top || n == top && w < topWord {
			topWord, top = w, n
		}
	}
	return fmt.Sprintf("%d:%d:%s:%d", total, len(counts), topWord, top)
}
