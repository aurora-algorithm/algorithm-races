package main

// The "work" kernels (kernels/SPEC.md section 11): image resize, compression, password hashing, CSV, word count.

import (
	"bufio"
	"bytes"
	"cmp"
	"compress/gzip"
	"crypto/rand"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/base64"
	"encoding/csv"
	"encoding/hex"
	"errors"
	"fmt"
	"image"
	"image/jpeg"
	"io"
	"os"
	"path/filepath"
	"runtime/debug"
	"strconv"
	"strings"

	"github.com/disintegration/imaging"
	kgzip "github.com/klauspost/compress/gzip"
	"github.com/klauspost/compress/zstd"
	"golang.org/x/crypto/argon2"
	"golang.org/x/crypto/bcrypt"
	"golang.org/x/image/draw"
)

const (
	photoFiles   = 8
	targetWidth  = 1080
	jpegQuality  = 80
	bcryptCost   = 10
	argon2Memory = 19456
	argon2Time   = 2
	argon2Lanes  = 1
	argon2KeyLen = 32
	refPassword  = "correct horse battery staple"
	bcryptRef    = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"
	argon2Ref    = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk"
)

type workKernel func(c config) (checksum string, timesMs []float64, extra map[string]any, err error)

var workKernels = map[string]workKernel{
	"image":      imageKernel(resizeDraw),
	"image-alt":  imageKernel(resizeImaging),
	"gzip":       compressKernel(gzipStd, gunzipStd),
	"gunzip":     decompressKernel(gzipStd, gunzipStd),
	"gzip-alt":   compressKernel(gzipKlauspost, gunzipKlauspost),
	"gunzip-alt": decompressKernel(gzipKlauspost, gunzipKlauspost),
	"zstd":       compressKernel(zstdCompress, zstdDecompress),
	"unzstd":     decompressKernel(zstdCompress, zstdDecompress),
	"bcrypt":     passwordKernel(bcryptHash, bcryptChecksum),
	"argon2id":   passwordKernel(argon2Hash, argon2Checksum),
	"csv":        fileKernel("orders-%d.csv", csvTotals, csvChecksum),
	"wordcount":  fileKernel("text-%d.txt", wordcount, wordcountChecksum),
}

// workLibraries names the modules the work kernels use, with the versions go.mod pinned.
func workLibraries() string {
	info, ok := debug.ReadBuildInfo()
	if !ok {
		return ""
	}
	var mods []string
	for _, dep := range info.Deps {
		mods = append(mods, dep.Path+" "+dep.Version)
	}
	return strings.Join(mods, ", ")
}

type resizer func(src image.Image, width, height int) image.Image

func resizeDraw(src image.Image, width, height int) image.Image {
	dst := image.NewRGBA(image.Rect(0, 0, width, height))
	draw.CatmullRom.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)
	return dst
}

func resizeImaging(src image.Image, width, height int) image.Image {
	return imaging.Resize(src, width, height, imaging.Lanczos)
}

func targetHeight(width, height int) int {
	return (height*targetWidth + width/2) / width
}

type imageResult struct {
	width, height, outBytes int
}

func convertAll(photos [][]byte, resize resizer) (imageResult, error) {
	var r imageResult
	for _, data := range photos {
		src, err := jpeg.Decode(bytes.NewReader(data))
		if err != nil {
			return r, err
		}
		b := src.Bounds()
		small := resize(src, targetWidth, targetHeight(b.Dx(), b.Dy()))
		var out bytes.Buffer
		if err := jpeg.Encode(&out, small, &jpeg.Options{Quality: jpegQuality}); err != nil {
			return r, err
		}
		r = imageResult{small.Bounds().Dx(), small.Bounds().Dy(), r.outBytes + out.Len()}
	}
	return r, nil
}

func imageKernel(resize resizer) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		photos := make([][]byte, c.size)
		for i := range photos {
			data, err := os.ReadFile(filepath.Join(c.data, "photos", fmt.Sprintf("photo-%d.jpg", i%photoFiles)))
			if err != nil {
				return "", nil, nil, err
			}
			photos[i] = data
		}
		var runErr error
		out, times := measure(c.repeats, same(photos), func(p [][]byte) imageResult {
			r, err := convertAll(p, resize)
			runErr = cmp.Or(runErr, err)
			return r
		})
		if runErr != nil {
			return "", nil, nil, runErr
		}
		checksum := fmt.Sprintf("%d:%dx%d", len(photos), out.width, out.height)
		return checksum, times, map[string]any{"out_bytes": out.outBytes}, nil
	}
}

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

func gzipKlauspost(raw []byte) ([]byte, error) {
	var out bytes.Buffer
	w, err := kgzip.NewWriterLevel(&out, 6)
	if err != nil {
		return nil, err
	}
	if _, err := w.Write(raw); err != nil {
		return nil, err
	}
	err = w.Close()
	return out.Bytes(), err
}

func gunzipKlauspost(packed []byte) ([]byte, error) {
	r, err := kgzip.NewReader(bytes.NewReader(packed))
	if err != nil {
		return nil, err
	}
	return io.ReadAll(r)
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

func passwords(n int) []string {
	pws := make([]string, n)
	for i := range pws {
		pws[i] = fmt.Sprintf("user-%d-password", i)
	}
	return pws
}

func passwordKernel(hash func(string) (string, error), check func([]string) string) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		var runErr error
		hashAll := func(pws []string) []string {
			out := make([]string, len(pws))
			for i, pw := range pws {
				h, err := hash(pw)
				runErr = cmp.Or(runErr, err)
				out[i] = h
			}
			return out
		}
		hashes, times := measure(c.repeats, same(passwords(c.size)), hashAll)
		if runErr != nil {
			return "", nil, nil, runErr
		}
		return check(hashes), times, nil, nil
	}
}

func bcryptHash(pw string) (string, error) {
	h, err := bcrypt.GenerateFromPassword([]byte(pw), bcryptCost)
	return string(h), err
}

func bcryptVerifies(hash, pw string) bool {
	return bcrypt.CompareHashAndPassword([]byte(hash), []byte(pw)) == nil
}

func bcryptChecksum(hashes []string) string {
	ok := 0
	for i, h := range hashes {
		cost, err := bcrypt.Cost([]byte(h))
		if err == nil && cost == bcryptCost && bcryptVerifies(h, fmt.Sprintf("user-%d-password", i)) {
			ok++
		}
	}
	return fmt.Sprintf("%d:%d:%d", len(hashes), ok, b2i(bcryptVerifies(bcryptRef, refPassword)))
}

func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}

var b64 = base64.RawStdEncoding

func argon2Prefix() string {
	return fmt.Sprintf("$argon2id$v=%d$m=%d,t=%d,p=%d$", argon2.Version, argon2Memory, argon2Time, argon2Lanes)
}

// argon2Hash returns the PHC string (x/crypto/argon2 only derives the key; the encoding is ours).
func argon2Hash(pw string) (string, error) {
	salt := make([]byte, 16)
	if _, err := rand.Read(salt); err != nil {
		return "", err
	}
	key := argon2.IDKey([]byte(pw), salt, argon2Time, argon2Memory, argon2Lanes, argon2KeyLen)
	return argon2Prefix() + b64.EncodeToString(salt) + "$" + b64.EncodeToString(key), nil
}

func argon2Verifies(phc, pw string) bool {
	parts := strings.Split(phc, "$")
	if len(parts) != 6 || parts[1] != "argon2id" || parts[2] != "v=19" {
		return false
	}
	var m, t uint32
	var p uint8
	if _, err := fmt.Sscanf(parts[3], "m=%d,t=%d,p=%d", &m, &t, &p); err != nil {
		return false
	}
	salt, err1 := b64.DecodeString(parts[4])
	want, err2 := b64.DecodeString(parts[5])
	if err := errors.Join(err1, err2); err != nil {
		return false
	}
	got := argon2.IDKey([]byte(pw), salt, t, m, p, uint32(len(want)))
	return subtle.ConstantTimeCompare(got, want) == 1
}

func argon2Checksum(hashes []string) string {
	ok := 0
	for i, h := range hashes {
		if strings.HasPrefix(h, argon2Prefix()) && argon2Verifies(h, fmt.Sprintf("user-%d-password", i)) {
			ok++
		}
	}
	return fmt.Sprintf("%d:%d:%d", len(hashes), ok, b2i(argon2Verifies(argon2Ref, refPassword)))
}

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
