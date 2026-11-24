// Race: zstd (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
	"github.com/klauspost/compress/zstd"
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

func compressKernel(compress, decompress codec) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		raw, err := readCorpus(c)
		if err != nil {
			return "", nil, nil, err
		}
		packed, times := measure(c.repeats, same(raw), must(compress))
		back, err := decompress(packed)
		if err != nil {
			return "", nil, nil, err
		}
		return plainChecksum(back), times, map[string]any{"compressed_bytes": len(packed)}, nil
	}
}

// zstdCompress turns off the content checksum, which klauspost writes by default and libzstd (every other row) does
// not, so Go neither writes nor verifies one.
func zstdCompress(raw []byte) ([]byte, error) {
	enc, err := zstd.NewWriter(nil, zstd.WithEncoderLevel(zstd.EncoderLevelFromZstd(3)), zstd.WithEncoderCRC(false))
	if err != nil {
		return nil, err
	}
	defer enc.Close()
	return enc.EncodeAll(raw, nil), nil
}

func zstdDecompress(packed []byte) ([]byte, error) {
	dec, err := zstd.NewReader(nil)
	if err != nil {
		return nil, err
	}
	defer dec.Close()
	return dec.DecodeAll(packed, nil)
}
