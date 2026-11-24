"""Benchmarks Game kernels, single-threaded (kernels/SPEC.md section 4). Floating-point operations keep the order
written here in every language so the printed results match to the last digit."""

import math

MIN_DEPTH = 4


def make_tree(depth: int):
    if depth == 0:
        return (None, None)
    depth -= 1
    return (make_tree(depth), make_tree(depth))


def check_tree(node) -> int:
    left, right = node
    if left is None:
        return 1
    return 1 + check_tree(left) + check_tree(right)


def depth_checks(depth: int, iterations: int) -> int:
    check = 0
    for _ in range(iterations):
        check += check_tree(make_tree(depth))
    return check


def binary_trees_lines(n: int, depth_run) -> str:
    max_depth = max(MIN_DEPTH + 2, n)
    stretch = max_depth + 1
    out = [f"stretch tree of depth {stretch}\t check: {check_tree(make_tree(stretch))}"]
    long_lived = make_tree(max_depth)
    for depth in range(MIN_DEPTH, max_depth + 1, 2):
        iterations = 1 << (max_depth - depth + MIN_DEPTH)
        out.append(f"{iterations}\t trees of depth {depth}\t check: {depth_run(depth, iterations)}")
    out.append(f"long lived tree of depth {max_depth}\t check: {check_tree(long_lived)}")
    return "\n".join(out) + "\n"


def binary_trees(n: int) -> str:
    return binary_trees_lines(n, depth_checks)


PI = 3.141592653589793
SOLAR_MASS = 4 * PI * PI
DAYS_PER_YEAR = 365.24

BODIES = (
    (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS),
    (
        4.84143144246472090e00,
        -1.16032004402742839e00,
        -1.03622044471123109e-01,
        1.66007664274403694e-03 * DAYS_PER_YEAR,
        7.69901118419740425e-03 * DAYS_PER_YEAR,
        -6.90460016972063023e-05 * DAYS_PER_YEAR,
        9.54791938424326609e-04 * SOLAR_MASS,
    ),
    (
        8.34336671824457987e00,
        4.12479856412430479e00,
        -4.03523417114321381e-01,
        -2.76742510726862411e-03 * DAYS_PER_YEAR,
        4.99852801234917238e-03 * DAYS_PER_YEAR,
        2.30417297573763929e-05 * DAYS_PER_YEAR,
        2.85885980666130812e-04 * SOLAR_MASS,
    ),
    (
        1.28943695621391310e01,
        -1.51111514016986312e01,
        -2.23307578892655734e-01,
        2.96460137564761618e-03 * DAYS_PER_YEAR,
        2.37847173959480950e-03 * DAYS_PER_YEAR,
        -2.96589568540237556e-05 * DAYS_PER_YEAR,
        4.36624404335156298e-05 * SOLAR_MASS,
    ),
    (
        1.53796971148509165e01,
        -2.59193146099879641e01,
        1.79258772950371181e-01,
        2.68067772490389322e-03 * DAYS_PER_YEAR,
        1.62824170038242295e-03 * DAYS_PER_YEAR,
        -9.51592254519715870e-05 * DAYS_PER_YEAR,
        5.15138902046611451e-05 * SOLAR_MASS,
    ),
)


class Body:
    __slots__ = ("mass", "vx", "vy", "vz", "x", "y", "z")

    def __init__(self, x, y, z, vx, vy, vz, mass) -> None:
        self.x, self.y, self.z, self.vx, self.vy, self.vz, self.mass = x, y, z, vx, vy, vz, mass


def offset_momentum(bodies: list[Body]) -> None:
    px = py = pz = 0.0
    for b in bodies:
        px += b.vx * b.mass
        py += b.vy * b.mass
        pz += b.vz * b.mass
    sun = bodies[0]
    sun.vx = -px / SOLAR_MASS
    sun.vy = -py / SOLAR_MASS
    sun.vz = -pz / SOLAR_MASS


def energy(bodies: list[Body]) -> float:
    e = 0.0
    for i, bi in enumerate(bodies):
        e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz)
        for bj in bodies[i + 1 :]:
            dx = bi.x - bj.x
            dy = bi.y - bj.y
            dz = bi.z - bj.z
            e -= (bi.mass * bj.mass) / math.sqrt(dx * dx + dy * dy + dz * dz)
    return e


def advance(bodies: list[Body], dt: float, steps: int) -> None:
    pairs = [(bodies[i], bodies[j]) for i in range(len(bodies)) for j in range(i + 1, len(bodies))]
    for _ in range(steps):
        for bi, bj in pairs:
            dx = bi.x - bj.x
            dy = bi.y - bj.y
            dz = bi.z - bj.z
            d2 = dx * dx + dy * dy + dz * dz
            mag = dt / (d2 * math.sqrt(d2))
            bim = bi.mass * mag
            bjm = bj.mass * mag
            bi.vx -= dx * bjm
            bi.vy -= dy * bjm
            bi.vz -= dz * bjm
            bj.vx += dx * bim
            bj.vy += dy * bim
            bj.vz += dz * bim
        for b in bodies:
            b.x += dt * b.vx
            b.y += dt * b.vy
            b.z += dt * b.vz


def nbody(steps: int) -> str:
    bodies = [Body(*b) for b in BODIES]
    offset_momentum(bodies)
    before = energy(bodies)
    advance(bodies, 0.01, steps)
    return f"{before:.9f};{energy(bodies):.9f}"


def mandelbrot_row(n: int, y: int) -> bytes:
    row = bytearray((n + 7) // 8)
    ci = 2.0 * y / n - 1.0
    for x in range(n):
        cr = 2.0 * x / n - 1.5
        zr = zi = tr = ti = 0.0
        i = 0
        while i < 50 and tr + ti <= 4.0:
            zi = 2.0 * zr * zi + ci
            zr = tr - ti + cr
            tr = zr * zr
            ti = zi * zi
            i += 1
        if tr + ti <= 4.0:
            row[x >> 3] |= 0x80 >> (x & 7)
    return bytes(row)


def mandelbrot(n: int) -> bytes:
    return b"".join(mandelbrot_row(n, y) for y in range(n))


def fannkuch(n: int) -> str:
    perm1 = list(range(n))
    count = [0] * n
    max_flips = 0
    checksum = 0
    perm_count = 0
    r = n
    while True:
        while r != 1:
            count[r - 1] = r
            r -= 1
        perm = perm1[:]
        flips = 0
        k = perm[0]
        while k != 0:
            perm[: k + 1] = perm[k::-1]
            flips += 1
            k = perm[0]
        max_flips = max(max_flips, flips)
        checksum += flips if perm_count % 2 == 0 else -flips
        while True:
            if r == n:
                return f"{checksum}:{max_flips}"
            perm0 = perm1[0]
            perm1[:r] = perm1[1 : r + 1]
            perm1[r] = perm0
            count[r] -= 1
            if count[r] > 0:
                break
            r += 1
        perm_count += 1


def eval_a(i: int, j: int) -> float:
    return 1.0 / ((i + j) * (i + j + 1) // 2 + i + 1)


def mul_av(u: list[float], n: int) -> list[float]:
    out = [0.0] * n
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += eval_a(i, j) * u[j]
        out[i] = s
    return out


def mul_atv(u: list[float], n: int) -> list[float]:
    out = [0.0] * n
    for i in range(n):
        s = 0.0
        for j in range(n):
            s += eval_a(j, i) * u[j]
        out[i] = s
    return out


def mul_atav(u: list[float], n: int) -> list[float]:
    return mul_atv(mul_av(u, n), n)


def spectral_norm(n: int) -> str:
    u = [1.0] * n
    v = u
    for _ in range(10):
        v = mul_atav(u, n)
        u = mul_atav(v, n)
    vbv = 0.0
    vv = 0.0
    for i in range(n):
        vbv += u[i] * v[i]
        vv += v[i] * v[i]
    return f"{math.sqrt(vbv / vv):.9f}"
