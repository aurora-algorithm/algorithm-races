# Race: nbody (python)
# Extracted from suite/python/bg.py — build and run via suite/run.py

import math


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
