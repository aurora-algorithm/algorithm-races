package main

import (
	"fmt"
	"math"
	"slices"
	"strings"
)

const minDepth = 4

type Node struct {
	Left, Right *Node
}

func makeTree(depth int) *Node {
	if depth == 0 {
		return &Node{}
	}
	return &Node{Left: makeTree(depth - 1), Right: makeTree(depth - 1)}
}

func (n *Node) Check() int {
	if n.Left == nil {
		return 1
	}
	return 1 + n.Left.Check() + n.Right.Check()
}

func depthChecks(depth, iterations int) int {
	check := 0
	for range iterations {
		check += makeTree(depth).Check()
	}
	return check
}

func binaryTreesLines(n int, depthRun func(depth, iterations int) int) string {
	maxDepth := max(minDepth+2, n)
	stretch := maxDepth + 1
	var sb strings.Builder
	fmt.Fprintf(&sb, "stretch tree of depth %d\t check: %d\n", stretch, makeTree(stretch).Check())
	longLived := makeTree(maxDepth)
	for depth := minDepth; depth <= maxDepth; depth += 2 {
		iterations := 1 << (maxDepth - depth + minDepth)
		fmt.Fprintf(&sb, "%d\t trees of depth %d\t check: %d\n", iterations, depth, depthRun(depth, iterations))
	}
	fmt.Fprintf(&sb, "long lived tree of depth %d\t check: %d\n", maxDepth, longLived.Check())
	return sb.String()
}

func binaryTrees(n int) string {
	return binaryTreesLines(n, depthChecks)
}

const (
	solarMass   = 4 * math.Pi * math.Pi
	daysPerYear = 365.24
)

type Body struct {
	x, y, z, vx, vy, vz, mass float64
}

// planet takes velocities in AU per day and the mass in solar masses. The scaling happens here, at run time in
// float64, because Go would multiply constant literals exactly at compile time and round once, which differs in
// the last bit from the reference (and every other language) for some of these values.
func planet(x, y, z, vx, vy, vz, mass float64) Body {
	return Body{
		x: x, y: y, z: z,
		vx: vx * daysPerYear, vy: vy * daysPerYear, vz: vz * daysPerYear,
		mass: mass * solarMass,
	}
}

func newBodies() []Body {
	return []Body{
		{mass: solarMass},
		planet( // Jupiter
			4.84143144246472090e00, -1.16032004402742839e00, -1.03622044471123109e-01,
			1.66007664274403694e-03, 7.69901118419740425e-03, -6.90460016972063023e-05,
			9.54791938424326609e-04,
		),
		planet( // Saturn
			8.34336671824457987e00, 4.12479856412430479e00, -4.03523417114321381e-01,
			-2.76742510726862411e-03, 4.99852801234917238e-03, 2.30417297573763929e-05,
			2.85885980666130812e-04,
		),
		planet( // Uranus
			1.28943695621391310e01, -1.51111514016986312e01, -2.23307578892655734e-01,
			2.96460137564761618e-03, 2.37847173959480950e-03, -2.96589568540237556e-05,
			4.36624404335156298e-05,
		),
		planet( // Neptune
			1.53796971148509165e01, -2.59193146099879641e01, 1.79258772950371181e-01,
			2.68067772490389322e-03, 1.62824170038242295e-03, -9.51592254519715870e-05,
			5.15138902046611451e-05,
		),
	}
}

func offsetMomentum(bodies []Body) {
	var px, py, pz float64
	for _, b := range bodies {
		px += b.vx * b.mass
		py += b.vy * b.mass
		pz += b.vz * b.mass
	}
	sun := &bodies[0]
	sun.vx = -px / solarMass
	sun.vy = -py / solarMass
	sun.vz = -pz / solarMass
}

func energy(bodies []Body) float64 {
	e := 0.0
	for i, bi := range bodies {
		e += 0.5 * bi.mass * (bi.vx*bi.vx + bi.vy*bi.vy + bi.vz*bi.vz)
		for _, bj := range bodies[i+1:] {
			dx := bi.x - bj.x
			dy := bi.y - bj.y
			dz := bi.z - bj.z
			e -= (bi.mass * bj.mass) / math.Sqrt(dx*dx+dy*dy+dz*dz)
		}
	}
	return e
}

func advance(bodies []Body, dt float64, steps int) {
	for range steps {
		for i := range bodies {
			bi := &bodies[i]
			for j := i + 1; j < len(bodies); j++ {
				bj := &bodies[j]
				dx := bi.x - bj.x
				dy := bi.y - bj.y
				dz := bi.z - bj.z
				d2 := dx*dx + dy*dy + dz*dz
				mag := dt / (d2 * math.Sqrt(d2))
				bim := bi.mass * mag
				bjm := bj.mass * mag
				bi.vx -= dx * bjm
				bi.vy -= dy * bjm
				bi.vz -= dz * bjm
				bj.vx += dx * bim
				bj.vy += dy * bim
				bj.vz += dz * bim
			}
		}
		for i := range bodies {
			b := &bodies[i]
			b.x += dt * b.vx
			b.y += dt * b.vy
			b.z += dt * b.vz
		}
	}
}

func nbody(steps int) string {
	bodies := newBodies()
	offsetMomentum(bodies)
	before := energy(bodies)
	advance(bodies, 0.01, steps)
	return fmt.Sprintf("%.9f;%.9f", before, energy(bodies))
}

func mandelbrotRow(n, y int, row []byte) {
	ci := 2.0*float64(y)/float64(n) - 1.0
	for x := range n {
		cr := 2.0*float64(x)/float64(n) - 1.5
		var zr, zi, tr, ti float64
		i := 0
		for i < 50 && tr+ti <= 4.0 {
			zi = 2.0*zr*zi + ci
			zr = tr - ti + cr
			tr = zr * zr
			ti = zi * zi
			i++
		}
		if tr+ti <= 4.0 {
			row[x>>3] |= 0x80 >> (x & 7)
		}
	}
}

func rowBytes(n int) int {
	return (n + 7) / 8
}

func mandelbrot(n int) []byte {
	width := rowBytes(n)
	bitmap := make([]byte, n*width)
	for y := range n {
		mandelbrotRow(n, y, bitmap[y*width:(y+1)*width])
	}
	return bitmap
}

func fannkuch(n int) string {
	perm1 := make([]int, n)
	for i := range perm1 {
		perm1[i] = i
	}
	perm := make([]int, n)
	count := make([]int, n)
	maxFlips, checksum, permCount := 0, 0, 0
	r := n
	for {
		for ; r != 1; r-- {
			count[r-1] = r
		}
		copy(perm, perm1)
		flips := 0
		for k := perm[0]; k != 0; k = perm[0] {
			slices.Reverse(perm[:k+1])
			flips++
		}
		maxFlips = max(maxFlips, flips)
		if permCount%2 == 0 {
			checksum += flips
		} else {
			checksum -= flips
		}
		for {
			if r == n {
				return fmt.Sprintf("%d:%d", checksum, maxFlips)
			}
			perm0 := perm1[0]
			copy(perm1[:r], perm1[1:r+1])
			perm1[r] = perm0
			count[r]--
			if count[r] > 0 {
				break
			}
			r++
		}
		permCount++
	}
}

func evalA(i, j int) float64 {
	return 1.0 / float64((i+j)*(i+j+1)/2+i+1)
}

func mulAv(u []float64) []float64 {
	out := make([]float64, len(u))
	for i := range out {
		s := 0.0
		for j, uj := range u {
			s += evalA(i, j) * uj
		}
		out[i] = s
	}
	return out
}

func mulAtv(u []float64) []float64 {
	out := make([]float64, len(u))
	for i := range out {
		s := 0.0
		for j, uj := range u {
			s += evalA(j, i) * uj
		}
		out[i] = s
	}
	return out
}

func mulAtAv(u []float64) []float64 {
	return mulAtv(mulAv(u))
}

func spectralNorm(n int) string {
	u := make([]float64, n)
	for i := range u {
		u[i] = 1.0
	}
	v := u
	for range 10 {
		v = mulAtAv(u)
		u = mulAtAv(v)
	}
	var vBv, vv float64
	for i := range n {
		vBv += u[i] * v[i]
		vv += v[i] * v[i]
	}
	return fmt.Sprintf("%.9f", math.Sqrt(vBv/vv))
}
