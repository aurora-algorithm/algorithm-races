// Race: image (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"bytes"
	"cmp"
	"fmt"
	"image"
	"image/jpeg"
	"os"
	"path/filepath"
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

type resizer func(src image.Image, width, height int) image.Image

func resizeDraw(src image.Image, width, height int) image.Image {
	dst := image.NewRGBA(image.Rect(0, 0, width, height))
	draw.CatmullRom.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)
	return dst
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
