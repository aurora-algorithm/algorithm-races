// Command kernels runs one CPU kernel from kernels/SPEC.md and prints one JSON line.
//
//	kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"runtime"
	"slices"
	"strconv"
	"strings"
	"time"
)

type config struct {
	size, repeats, threads int
	data                   string
}

type kernel func(c config) (checksum string, timesMs []float64, err error)

// measure runs the kernel `repeats` times; fresh builds each repeat's input outside the timed region.
func measure[In, Out any](repeats int, fresh func() In, run func(In) Out) (Out, []float64) {
	var out Out
	times := make([]float64, 0, repeats)
	for range repeats {
		in := fresh()
		start := time.Now()
		out = run(in)
		elapsed := float64(time.Since(start).Nanoseconds()) / 1e6
		times = append(times, math.Round(elapsed*1000)/1000)
	}
	return out, times
}

func same[T any](v T) func() T {
	return func() T { return v }
}

func formatHash(h uint32) string {
	return strconv.FormatUint(uint64(h), 10)
}

func sortedChecksum(a []int64) string {
	return formatHash(h31(a))
}

func stringsChecksum(a []string) string {
	return formatHash(fnv1a([]byte(strings.Join(a, "\n") + "\n")))
}

func bytesChecksum(b []byte) string {
	return formatHash(fnv1a(b))
}

func textChecksum(s string) string {
	return formatHash(fnv1a([]byte(s)))
}

func identity(s string) string {
	return s
}

func sortKernel(sort func([]int64) []int64) kernel {
	return func(c config) (string, []float64, error) {
		base := ints(c.size)
		out, times := measure(c.repeats, func() []int64 { return slices.Clone(base) }, sort)
		return sortedChecksum(out), times, nil
	}
}

// sizeKernel wraps a kernel whose only input is the size.
func sizeKernel[Out any](run func(n int) Out, check func(Out) string) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(c.size), run)
		return check(out), times, nil
	}
}

func threadedKernel[Out any](run func(n, threads int) Out, check func(Out) string) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(c.size), func(n int) Out { return run(n, c.threads) })
		return check(out), times, nil
	}
}

func sortStrKernel(c config) (string, []float64, error) {
	base := randomStrings(c.size)
	sortStrings := func(a []string) []string {
		slices.Sort(a)
		return a
	}
	out, times := measure(c.repeats, func() []string { return slices.Clone(base) }, sortStrings)
	return stringsChecksum(out), times, nil
}

func regexKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(logText(c.size)), regexLog)
	return out, times, nil
}

func hashmapKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(newHashmapInput(c.size)), hashmap)
	return out, times, nil
}

func sha256Kernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(shaBuffer(c.size)), sha256Hex)
	return out, times, nil
}

func matmulKernel(c config) (string, []float64, error) {
	a, b := matrices(c.size)
	out, times := measure(c.repeats, same(matrixInput{a: a, b: b, n: c.size}), matmul)
	return matmulChecksum(out), times, nil
}

func jsonKernel(c config) (string, []float64, error) {
	text, err := os.ReadFile(filepath.Join(c.data, fmt.Sprintf("orders-%d.json", c.size)))
	if err != nil {
		return "", nil, err
	}
	var runErr error
	roundtrip := func(text []byte) jsonResult {
		r, err := jsonRoundtrip(text)
		if err != nil {
			runErr = err
		}
		return r
	}
	out, times := measure(c.repeats, same(text), roundtrip)
	if runErr != nil {
		return "", nil, runErr
	}
	checksum, err := jsonChecksum(out, text)
	return checksum, times, err
}

var kernels = map[string]kernel{
	"sort-int": sortKernel(func(a []int64) []int64 {
		slices.Sort(a)
		return a
	}),
	"sort-str":        sortStrKernel,
	"quicksort":       sortKernel(runQuicksort),
	"mergesort":       sortKernel(runMergesort),
	"heapsort":        sortKernel(runHeapsort),
	"radixsort":       sortKernel(radixSort),
	"binary-trees":    sizeKernel(binaryTrees, textChecksum),
	"nbody":           sizeKernel(nbody, identity),
	"mandelbrot":      sizeKernel(mandelbrot, bytesChecksum),
	"fannkuch":        sizeKernel(fannkuch, identity),
	"spectral-norm":   sizeKernel(spectralNorm, identity),
	"regex-log":       regexKernel,
	"hashmap":         hashmapKernel,
	"sha256":          sha256Kernel,
	"sieve":           sizeKernel(sieve, identity),
	"matmul":          matmulKernel,
	"json":            jsonKernel,
	"mergesort-mt":    mergesortMTKernel,
	"mandelbrot-mt":   threadedKernel(mandelbrotMT, bytesChecksum),
	"binary-trees-mt": threadedKernel(binaryTreesMT, textChecksum),
}

func mergesortMTKernel(c config) (string, []float64, error) {
	base := ints(c.size)
	sort := func(a []int64) []int64 { return mergesortMT(a, c.threads) }
	out, times := measure(c.repeats, func() []int64 { return slices.Clone(base) }, sort)
	return sortedChecksum(out), times, nil
}

type report struct {
	Kernel   string         `json:"kernel"`
	Size     int            `json:"size"`
	Threads  int            `json:"threads"`
	Checksum string         `json:"checksum"`
	TimesMs  []float64      `json:"times_ms"`
	Runtime  string         `json:"runtime"`
	Extra    map[string]any `json:"extra,omitempty"`
}

func parseArgs(args []string) (string, config, error) {
	if len(args) < 1 || strings.HasPrefix(args[0], "-") {
		return "", config{}, fmt.Errorf("usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]")
	}
	fs := flag.NewFlagSet("kernels", flag.ContinueOnError)
	var c config
	fs.IntVar(&c.size, "size", 0, "input size (required)")
	fs.IntVar(&c.repeats, "repeats", 1, "runs in this process")
	fs.IntVar(&c.threads, "threads", 1, "worker goroutines for the -mt kernels")
	fs.StringVar(&c.data, "data", ".", "folder holding orders-<N>.json")
	if err := fs.Parse(args[1:]); err != nil {
		return "", config{}, err
	}
	if c.size <= 0 || c.repeats <= 0 || c.threads <= 0 {
		return "", config{}, fmt.Errorf("--size, --repeats and --threads must be positive")
	}
	return args[0], c, nil
}

func run() int {
	name, c, err := parseArgs(os.Args[1:])
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return 2
	}
	rt := fmt.Sprintf("%s GOMAXPROCS=%d", runtime.Version(), runtime.GOMAXPROCS(0))
	var checksum string
	var times []float64
	var extra map[string]any
	if k, ok := kernels[name]; ok {
		checksum, times, err = k(c)
	} else if w, ok := workKernels[name]; ok {
		checksum, times, extra, err = w(c)
		rt += " " + workLibraries()
	} else if loop, ok := ssearchKernels[name]; ok {
		checksum, times, extra, err = ssearchKernel(loop)(c)
	} else if a, ok := acKernels[name]; ok {
		checksum, times, extra, err = a(c)
	} else if name == "dijkstra" {
		checksum, times, extra, err = dijkstraKernel(c)
	} else if p, ok := puzzleKernels[name]; ok {
		checksum, times, err = p(c)
	} else {
		fmt.Fprintf(os.Stderr, "unknown kernel %s\n", name)
		return 2
	}
	if err != nil {
		fmt.Fprintf(os.Stderr, "%s: %v\n", name, err)
		return 1
	}
	line, err := json.Marshal(report{
		Kernel:   name,
		Size:     c.size,
		Threads:  c.threads,
		Checksum: checksum,
		TimesMs:  times,
		Runtime:  rt,
		Extra:    extra,
	})
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return 1
	}
	fmt.Println(string(line))
	return 0
}

func main() {
	os.Exit(run())
}
