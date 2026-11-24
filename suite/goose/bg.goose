// Benchmarks Game kernels, single-threaded (kernels/SPEC.md section 4). Floating-point operations keep the order
// of python/bg.py, and the generated C turns off contraction into fused multiply-adds, so the digits match.

import std;
import math;

let MIN_DEPTH = 4;

// A tree is built into the pool its caller owns, one push per node: Goose has no heap, so the pool is how a
// program allocates nodes, and dropping the pool at the end of its scope frees the whole tree at once.
struct TreeNode { left: TreeNode&<u32>?, right: TreeNode&<u32>? }

recursive fn make_tree(pool: TreeNode[>..]&, depth: i64) -> TreeNode& {
    if depth == 0 { return pool.push(TreeNode {}); }
    let left:  TreeNode& = make_tree(pool, depth - 1);
    let right: TreeNode& = make_tree(pool, depth - 1);
    return pool.push(TreeNode { left: left, right: right });
}

recursive fn check_tree(node: TreeNode&) -> i64 {
    let left = node.left;
    guard left else { return 1; }
    let right = node.right;
    guard right else { return 1; }
    return 1 + check_tree(left) + check_tree(right);
}

fn tree_check(depth: i64) -> i64 {
    var pool: TreeNode[>..] = [];
    return check_tree(make_tree(pool, depth));
}

fn binary_trees(n: i64, out: u8[>..]&) {
    let max_depth = max(MIN_DEPTH + 2, n);
    let stretch = max_depth + 1;
    format(out, "stretch tree of depth ", stretch, "\t check: ", tree_check(stretch), "\n");
    var long_pool: TreeNode[>..] = [];
    let long_lived: TreeNode& = make_tree(long_pool, max_depth);
    var depth = MIN_DEPTH;
    while depth <= max_depth {
        let iterations = 1 << (max_depth - depth + MIN_DEPTH);
        var check = 0;
        for i in iterations { check += tree_check(depth); }
        format(out, iterations, "\t trees of depth ", depth, "\t check: ", check, "\n");
        depth += 2;
    }
    format(out, "long lived tree of depth ", max_depth, "\t check: ", check_tree(long_lived), "\n");
}

let SOLAR_MASS = 4.0 * PI * PI;
let DAYS_PER_YEAR = 365.24;

struct Body { x: f64, y: f64, z: f64, vx: f64, vy: f64, vz: f64, mass: f64 }

fn bodies() -> Body[5] {
    var b: Body[5] = [
        Body { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS },
        Body {
            4.84143144246472090e00, -1.16032004402742839e00, -1.03622044471123109e-01,
            1.66007664274403694e-03 * DAYS_PER_YEAR, 7.69901118419740425e-03 * DAYS_PER_YEAR,
            -6.90460016972063023e-05 * DAYS_PER_YEAR, 9.54791938424326609e-04 * SOLAR_MASS,
        },
        Body {
            8.34336671824457987e00, 4.12479856412430479e00, -4.03523417114321381e-01,
            -2.76742510726862411e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR,
            2.30417297573763929e-05 * DAYS_PER_YEAR, 2.85885980666130812e-04 * SOLAR_MASS,
        },
        Body {
            1.28943695621391310e01, -1.51111514016986312e01, -2.23307578892655734e-01,
            2.96460137564761618e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR,
            -2.96589568540237556e-05 * DAYS_PER_YEAR, 4.36624404335156298e-05 * SOLAR_MASS,
        },
        Body {
            1.53796971148509165e01, -2.59193146099879641e01, 1.79258772950371181e-01,
            2.68067772490389322e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR,
            -9.51592254519715870e-05 * DAYS_PER_YEAR, 5.15138902046611451e-05 * SOLAR_MASS,
        },
    ];
    var px = 0.0;
    var py = 0.0;
    var pz = 0.0;
    for p in b {
        px += p.vx * p.mass;
        py += p.vy * p.mass;
        pz += p.vz * p.mass;
    }
    b[0].vx = -px / SOLAR_MASS;
    b[0].vy = -py / SOLAR_MASS;
    b[0].vz = -pz / SOLAR_MASS;
    return b;
}

fn energy(b: Body[5]&) -> f64 {
    var e = 0.0;
    for i in 5 {
        let bi = b[i];
        e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz);
        for j in i + 1..5 {
            let bj = b[j];
            let dx = bi.x - bj.x;
            let dy = bi.y - bj.y;
            let dz = bi.z - bj.z;
            e -= (bi.mass * bj.mass) / sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}

fn advance(b: Body[5]&, dt: f64, steps: i64) {
    for s in steps {
        for i in 5 {
            let bi .= b[i];
            for j in i + 1..5 {
                let bj .= b[j];
                let dx = bi.x - bj.x;
                let dy = bi.y - bj.y;
                let dz = bi.z - bj.z;
                let d2 = dx * dx + dy * dy + dz * dz;
                let mag = dt / (d2 * sqrt(d2));
                let bim = bi.mass * mag;
                let bjm = bj.mass * mag;
                bi.vx -= dx * bjm;
                bi.vy -= dy * bjm;
                bi.vz -= dz * bjm;
                bj.vx += dx * bim;
                bj.vy += dy * bim;
                bj.vz += dz * bim;
            }
        }
        for &p in b {
            p.x += dt * p.vx;
            p.y += dt * p.vy;
            p.z += dt * p.vz;
        }
    }
}

fn nbody(steps: i64, out: u8[>..]&) {
    var b = bodies();
    let before = energy(b);
    advance(b, 0.01, steps);
    format_flt(out, before, 9);
    out.push(';');
    format_flt(out, energy(b), 9);
}

fn mandelbrot(n: i64) -> u8[>..] {
    let row_bytes = (n + 7) / 8;
    var out: u8[>..] = [];
    out.resize(row_bytes * n, 0);
    for y in n {
        let ci = 2.0 * y / n - 1.0;
        let row = y * row_bytes;
        for x in n {
            let cr = 2.0 * x / n - 1.5;
            var zr = 0.0;
            var zi = 0.0;
            var tr = 0.0;
            var ti = 0.0;
            var i = 0;
            while i < 50 && tr + ti <= 4.0 {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i++;
            }
            if tr + ti <= 4.0 { out[row + (x >> 3)] |= (0x80 >> (x & 7)) as! u8; }
        }
    }
    return out;
}

fn fannkuch(n: i64, out: u8[>..]&) {
    var perm1: i64[>..] = [];
    var count: i64[>..] = [];
    var perm:  i64[>..] = [];
    for i in n {
        perm1.push(i);
        count.push(0);
        perm.push(0);
    }
    var max_flips = 0;
    var checksum = 0;
    var perm_count = 0;
    var r = n;
    loop {
        while r != 1 {
            count[r - 1] = r;
            r--;
        }
        copy_into(perm, perm1);
        var flips = 0;
        var k = perm[0];
        while k != 0 {
            reverse(perm[..k + 1]);
            flips++;
            k = perm[0];
        }
        max_flips = max(max_flips, flips);
        checksum += if perm_count % 2 == 0 { flips } else { -flips };
        loop {
            if r == n {
                format(out, checksum, ":", max_flips);
                return;
            }
            let perm0 = perm1[0];
            for i in r { perm1[i] = perm1[i + 1]; }
            perm1[r] = perm0;
            count[r]--;
            if count[r] > 0 { break; }
            r++;
        }
        perm_count++;
    }
}

fn eval_a(i: i64, j: i64) -> f64 { 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1) }

fn mul_av(u: f64[:], out: f64[:]) {
    let n = u.len;
    for i in n {
        var s = 0.0;
        for j in n { s += eval_a(i, j) * u[j]; }
        out[i] = s;
    }
}

fn mul_atv(u: f64[:], out: f64[:]) {
    let n = u.len;
    for i in n {
        var s = 0.0;
        for j in n { s += eval_a(j, i) * u[j]; }
        out[i] = s;
    }
}

fn mul_atav(u: f64[:], out: f64[:], tmp: f64[:]) {
    mul_av(u, tmp);
    mul_atv(tmp, out);
}

fn spectral_norm(n: i64, out: u8[>..]&) {
    var u:   f64[>..] = [];
    var v:   f64[>..] = [];
    var tmp: f64[>..] = [];
    u.resize(n, 1.0);
    v.resize(n, 0.0);
    tmp.resize(n, 0.0);
    for i in 10 {
        mul_atav(u, v, tmp);
        mul_atav(v, u, tmp);
    }
    var vbv = 0.0;
    var vv = 0.0;
    for i in n {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    format_flt(out, sqrt(vbv / vv), 9);
}
