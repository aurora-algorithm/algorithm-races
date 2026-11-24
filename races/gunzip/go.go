// Race: gunzip (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"bytes"
	"compress/gzip"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"io"
	"os"
	"path/filepath"
)

type workKernel func(c config) (checksum string, timesMs []float64, extra map[string]any, err error)

func readCorpus(c config) ([]byte, error) {
	return os.ReadFile(filepath.Join(c.data, fmt.Sprintf("corpus-%d.bin", c.size)))
}

func plainChecksum(raw []byte) string {
	sum := sha256.Sum256(raw)
	return fmt.Sprintf("%d:%s", len(raw), hex.EncodeToString(sum[:]))
}

type codec func([]byte) ([]byte, error)

// must turns a codec error into a panic: the codecs only fail on our own data if the library is broken.
func must(f codec) func([]byte) []byte {
	return func(in []byte) []byte {
		out, err := f(in)
		if err != nil {
			panic(err)
		}
		return out
	}
}

func decompressKernel(compress, decompress codec) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		raw, err := readCorpus(c)
		if err != nil {
			return "", nil, nil, err
		}
		packed, err := compress(raw)
		if err != nil {
			return "", nil, nil, err
		}
		back, times := measure(c.repeats, same(packed), must(decompress))
		return plainChecksum(back), times, nil, nil
	}
}

func gzipStd(raw []byte) ([]byte, error) {
	var out bytes.Buffer
	w, err := gzip.NewWriterLevel(&out, 6)
	if err != nil {
		return nil, err
	}
	if _, err := w.Write(raw); err != nil {
		return nil, err
	}
	err = w.Close()
	return out.Bytes(), err
}

func gunzipStd(packed []byte) ([]byte, error) {
	r, err := gzip.NewReader(bytes.NewReader(packed))
	if err != nil {
		return nil, err
	}
	return io.ReadAll(r)
}
