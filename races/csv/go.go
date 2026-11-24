// Race: csv (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"bufio"
	"cmp"
	"encoding/csv"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strconv"
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

type totals struct {
	rows int
	sums map[int]int64
}

func csvTotals(path string) (totals, error) {
	f, err := os.Open(path)
	if err != nil {
		return totals{}, err
	}
	defer f.Close()
	r := csv.NewReader(bufio.NewReader(f))
	header, err := r.Read()
	if err != nil {
		return totals{}, err
	}
	cust, total := indexOf(header, "customer_id"), indexOf(header, "total_cents")
	t := totals{sums: map[int]int64{}}
	for {
		rec, err := r.Read()
		if err == io.EOF {
			return t, nil
		}
		if err != nil {
			return t, err
		}
		id, err1 := strconv.Atoi(rec[cust])
		cents, err2 := strconv.ParseInt(rec[total], 10, 64)
		if err := errors.Join(err1, err2); err != nil {
			return t, err
		}
		t.rows++
		t.sums[id] += cents
	}
}

func indexOf(header []string, name string) int {
	for i, h := range header {
		if h == name {
			return i
		}
	}
	return -1
}

func csvChecksum(t totals) string {
	var sum, top int64
	topID := 0
	for id, s := range t.sums {
		sum += s
		if s > top || s == top && id < topID {
			topID, top = id, s
		}
	}
	return fmt.Sprintf("%d:%d:%d:%d:%d", t.rows, len(t.sums), sum, topID, top)
}
