// Race: nbody (goose)
// Extracted from suite/goose/bg.goose — build and run via suite/run.py

import std;
import math;

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
