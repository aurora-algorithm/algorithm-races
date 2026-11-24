// Race: regex-log (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main
import (
	"fmt"
	"regexp"
	"strconv"
)

var logPattern = regexp.MustCompile(`(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])`)

func regexLog(text string) string {
	count, ids := 0, 0
	for _, m := range logPattern.FindAllStringSubmatch(text, -1) {
		id, _ := strconv.Atoi(m[2]) // the pattern only matches digits
		count++
		ids += id
	}
	return fmt.Sprintf("%d:%d", count, ids)
}
