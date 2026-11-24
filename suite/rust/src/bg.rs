//! Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps
//! the operation order of the spec so every language prints the same digits.

use std::f64::consts::PI;
use std::fmt::Write;

pub const MIN_DEPTH: u32 = 4;

pub struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

pub fn make_tree(depth: u32) -> Box<Node> {
    if depth == 0 {
        return Box::new(Node {
            left: None,
            right: None,
        });
    }
    Box::new(Node {
        left: Some(make_tree(depth - 1)),
        right: Some(make_tree(depth - 1)),
    })
}

pub fn check_tree(node: &Node) -> u64 {
    match (&node.left, &node.right) {
        (Some(left), Some(right)) => 1 + check_tree(left) + check_tree(right),
        _ => 1,
    }
}

pub fn depth_checks(depth: u32, iterations: u64) -> u64 {
    (0..iterations).map(|_| check_tree(&make_tree(depth))).sum()
}

/// The Benchmarks Game output; `depth_run(depth, iterations)` builds and checks the trees of one depth.
pub fn binary_trees_lines(n: u32, depth_run: impl Fn(u32, u64) -> u64) -> String {
    let max_depth = (MIN_DEPTH + 2).max(n);
    let stretch = max_depth + 1;
    let mut out = String::new();
    let stretch_check = check_tree(&make_tree(stretch));
    writeln!(
        out,
        "stretch tree of depth {stretch}\t check: {stretch_check}"
    )
    .unwrap();
    let long_lived = make_tree(max_depth);
    for depth in (MIN_DEPTH..=max_depth).step_by(2) {
        let iterations = 1u64 << (max_depth - depth + MIN_DEPTH);
        let check = depth_run(depth, iterations);
        writeln!(
            out,
            "{iterations}\t trees of depth {depth}\t check: {check}"
        )
        .unwrap();
    }
    let long_check = check_tree(&long_lived);
    writeln!(
        out,
        "long lived tree of depth {max_depth}\t check: {long_check}"
    )
    .unwrap();
    out
}

pub fn binary_trees(n: u32) -> String {
    binary_trees_lines(n, depth_checks)
}

const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;

#[derive(Clone, Copy)]
struct Body {
    x: f64,
    y: f64,
    z: f64,
    vx: f64,
    vy: f64,
    vz: f64,
    mass: f64,
}

// The Benchmarks Game's literals, kept digit for digit (they round to the same f64 values).
#[allow(clippy::excessive_precision)]
const BODIES: [Body; 5] = [
    // Sun
    Body {
        x: 0.0,
        y: 0.0,
        z: 0.0,
        vx: 0.0,
        vy: 0.0,
        vz: 0.0,
        mass: SOLAR_MASS,
    },
    // Jupiter
    Body {
        x: 4.84143144246472090e00,
        y: -1.16032004402742839e00,
        z: -1.03622044471123109e-01,
        vx: 1.66007664274403694e-03 * DAYS_PER_YEAR,
        vy: 7.69901118419740425e-03 * DAYS_PER_YEAR,
        vz: -6.90460016972063023e-05 * DAYS_PER_YEAR,
        mass: 9.54791938424326609e-04 * SOLAR_MASS,
    },
    // Saturn
    Body {
        x: 8.34336671824457987e00,
        y: 4.12479856412430479e00,
        z: -4.03523417114321381e-01,
        vx: -2.76742510726862411e-03 * DAYS_PER_YEAR,
        vy: 4.99852801234917238e-03 * DAYS_PER_YEAR,
        vz: 2.30417297573763929e-05 * DAYS_PER_YEAR,
        mass: 2.85885980666130812e-04 * SOLAR_MASS,
    },
    // Uranus
    Body {
        x: 1.28943695621391310e01,
        y: -1.51111514016986312e01,
        z: -2.23307578892655734e-01,
        vx: 2.96460137564761618e-03 * DAYS_PER_YEAR,
        vy: 2.37847173959480950e-03 * DAYS_PER_YEAR,
        vz: -2.96589568540237556e-05 * DAYS_PER_YEAR,
        mass: 4.36624404335156298e-05 * SOLAR_MASS,
    },
    // Neptune
    Body {
        x: 1.53796971148509165e01,
        y: -2.59193146099879641e01,
        z: 1.79258772950371181e-01,
        vx: 2.68067772490389322e-03 * DAYS_PER_YEAR,
        vy: 1.62824170038242295e-03 * DAYS_PER_YEAR,
        vz: -9.51592254519715870e-05 * DAYS_PER_YEAR,
        mass: 5.15138902046611451e-05 * SOLAR_MASS,
    },
];

fn offset_momentum(bodies: &mut [Body]) {
    let (mut px, mut py, mut pz) = (0.0, 0.0, 0.0);
    for b in bodies.iter() {
        px += b.vx * b.mass;
        py += b.vy * b.mass;
        pz += b.vz * b.mass;
    }
    let sun = &mut bodies[0];
    sun.vx = -px / SOLAR_MASS;
    sun.vy = -py / SOLAR_MASS;
    sun.vz = -pz / SOLAR_MASS;
}

fn energy(bodies: &[Body]) -> f64 {
    let mut e = 0.0;
    for (i, bi) in bodies.iter().enumerate() {
        e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz);
        for bj in &bodies[i + 1..] {
            let dx = bi.x - bj.x;
            let dy = bi.y - bj.y;
            let dz = bi.z - bj.z;
            e -= (bi.mass * bj.mass) / (dx * dx + dy * dy + dz * dz).sqrt();
        }
    }
    e
}

fn advance(bodies: &mut [Body], dt: f64, steps: usize) {
    for _ in 0..steps {
        for i in 0..bodies.len() {
            let (head, tail) = bodies.split_at_mut(i + 1);
            let bi = &mut head[i];
            for bj in tail.iter_mut() {
                let dx = bi.x - bj.x;
                let dy = bi.y - bj.y;
                let dz = bi.z - bj.z;
                let d2 = dx * dx + dy * dy + dz * dz;
                let mag = dt / (d2 * d2.sqrt());
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
        for b in bodies.iter_mut() {
            b.x += dt * b.vx;
            b.y += dt * b.vy;
            b.z += dt * b.vz;
        }
    }
}

pub fn nbody(steps: usize) -> String {
    let mut bodies = BODIES;
    offset_momentum(&mut bodies);
    let before = energy(&bodies);
    advance(&mut bodies, 0.01, steps);
    format!("{before:.9};{:.9}", energy(&bodies))
}

/// One bitmap row: bit x (MSB first) is set when point (x, y) stays in the set for 50 iterations.
pub fn mandelbrot_row(n: usize, y: usize, row: &mut [u8]) {
    let ci = 2.0 * y as f64 / n as f64 - 1.0;
    for x in 0..n {
        let cr = 2.0 * x as f64 / n as f64 - 1.5;
        let (mut zr, mut zi, mut tr, mut ti) = (0.0, 0.0, 0.0, 0.0);
        let mut i = 0;
        while i < 50 && tr + ti <= 4.0 {
            zi = 2.0 * zr * zi + ci;
            zr = tr - ti + cr;
            tr = zr * zr;
            ti = zi * zi;
            i += 1;
        }
        if tr + ti <= 4.0 {
            row[x >> 3] |= 0x80 >> (x & 7);
        }
    }
}

pub fn row_bytes(n: usize) -> usize {
    n.div_ceil(8)
}

pub fn mandelbrot(n: usize) -> Vec<u8> {
    let mut bitmap = vec![0u8; n * row_bytes(n)];
    for (y, row) in bitmap.chunks_mut(row_bytes(n)).enumerate() {
        mandelbrot_row(n, y, row);
    }
    bitmap
}

pub fn fannkuch(n: usize) -> String {
    let mut perm1: Vec<usize> = (0..n).collect();
    let mut count = vec![0usize; n];
    let mut max_flips = 0;
    let mut checksum: i64 = 0;
    let mut perm_count = 0u64;
    let mut perm = vec![0usize; n];
    let mut r = n;
    loop {
        while r != 1 {
            count[r - 1] = r;
            r -= 1;
        }
        perm.copy_from_slice(&perm1);
        let mut flips = 0;
        let mut k = perm[0];
        while k != 0 {
            perm[..=k].reverse();
            flips += 1;
            k = perm[0];
        }
        max_flips = max_flips.max(flips);
        checksum += if perm_count.is_multiple_of(2) {
            flips
        } else {
            -flips
        };
        loop {
            if r == n {
                return format!("{checksum}:{max_flips}");
            }
            perm1[..=r].rotate_left(1);
            count[r] -= 1;
            if count[r] > 0 {
                break;
            }
            r += 1;
        }
        perm_count += 1;
    }
}

fn eval_a(i: usize, j: usize) -> f64 {
    1.0 / ((i + j) * (i + j + 1) / 2 + i + 1) as f64
}

fn mul_av(u: &[f64]) -> Vec<f64> {
    let n = u.len();
    (0..n)
        .map(|i| {
            let mut s = 0.0;
            for (j, uj) in u.iter().enumerate() {
                s += eval_a(i, j) * uj;
            }
            s
        })
        .collect()
}

fn mul_atv(u: &[f64]) -> Vec<f64> {
    let n = u.len();
    (0..n)
        .map(|i| {
            let mut s = 0.0;
            for (j, uj) in u.iter().enumerate() {
                s += eval_a(j, i) * uj;
            }
            s
        })
        .collect()
}

fn mul_atav(u: &[f64]) -> Vec<f64> {
    mul_atv(&mul_av(u))
}

pub fn spectral_norm(n: usize) -> String {
    let mut u = vec![1.0; n];
    let mut v = Vec::new();
    for _ in 0..10 {
        v = mul_atav(&u);
        u = mul_atav(&v);
    }
    let (mut vbv, mut vv) = (0.0, 0.0);
    for (ui, vi) in u.iter().zip(&v) {
        vbv += ui * vi;
        vv += vi * vi;
    }
    format!("{:.9}", (vbv / vv).sqrt())
}
