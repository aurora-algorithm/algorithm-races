// Race: nbody (js)
// Extracted from suite/js/bg.mjs — build and run via suite/run.py

const SOLAR_MASS = 4 * Math.PI * Math.PI;

const DAYS_PER_YEAR = 365.24;

class Body {
	constructor(x, y, z, vx, vy, vz, mass) {
		Object.assign(this, { x, y, z, vx, vy, vz, mass });
	}
}

const BODIES = [
	[0, 0, 0, 0, 0, 0, SOLAR_MASS],
	[
		4.841431442464721,
		-1.1603200440274284,
		-0.10362204447112311,
		0.001660076642744037 * DAYS_PER_YEAR,
		0.007699011184197404 * DAYS_PER_YEAR,
		-0.0000690460016972063 * DAYS_PER_YEAR,
		0.0009547919384243266 * SOLAR_MASS,
	],
	[
		8.34336671824458,
		4.124798564124305,
		-0.4035234171143214,
		-0.002767425107268624 * DAYS_PER_YEAR,
		0.004998528012349172 * DAYS_PER_YEAR,
		0.000023041729757376393 * DAYS_PER_YEAR,
		0.0002858859806661308 * SOLAR_MASS,
	],
	[
		12.894369562139131,
		-15.111151401698631,
		-0.22330757889265573,
		0.002964601375647616 * DAYS_PER_YEAR,
		0.0023784717395948095 * DAYS_PER_YEAR,
		-0.000029658956854023756 * DAYS_PER_YEAR,
		0.00004366244043351563 * SOLAR_MASS,
	],
	[
		15.379697114850917,
		-25.919314609987964,
		0.17925877295037118,
		0.0026806777249038932 * DAYS_PER_YEAR,
		0.001628241700382423 * DAYS_PER_YEAR,
		-0.00009515922545197159 * DAYS_PER_YEAR,
		0.000051513890204661145 * SOLAR_MASS,
	],
];

function offsetMomentum(bodies) {
	let px = 0;
	let py = 0;
	let pz = 0;
	for (const b of bodies) {
		px += b.vx * b.mass;
		py += b.vy * b.mass;
		pz += b.vz * b.mass;
	}
	const sun = bodies[0];
	sun.vx = -px / SOLAR_MASS;
	sun.vy = -py / SOLAR_MASS;
	sun.vz = -pz / SOLAR_MASS;
}

function energy(bodies) {
	let e = 0;
	for (let i = 0; i < bodies.length; i++) {
		const bi = bodies[i];
		e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz);
		for (let j = i + 1; j < bodies.length; j++) {
			const bj = bodies[j];
			const dx = bi.x - bj.x;
			const dy = bi.y - bj.y;
			const dz = bi.z - bj.z;
			e -= (bi.mass * bj.mass) / Math.sqrt(dx * dx + dy * dy + dz * dz);
		}
	}
	return e;
}

function advance(bodies, dt, steps) {
	for (let step = 0; step < steps; step++) {
		for (let i = 0; i < bodies.length; i++) {
			const bi = bodies[i];
			for (let j = i + 1; j < bodies.length; j++) {
				const bj = bodies[j];
				const dx = bi.x - bj.x;
				const dy = bi.y - bj.y;
				const dz = bi.z - bj.z;
				const d2 = dx * dx + dy * dy + dz * dz;
				const mag = dt / (d2 * Math.sqrt(d2));
				const bim = bi.mass * mag;
				const bjm = bj.mass * mag;
				bi.vx -= dx * bjm;
				bi.vy -= dy * bjm;
				bi.vz -= dz * bjm;
				bj.vx += dx * bim;
				bj.vy += dy * bim;
				bj.vz += dz * bim;
			}
		}
		for (const b of bodies) {
			b.x += dt * b.vx;
			b.y += dt * b.vy;
			b.z += dt * b.vz;
		}
	}
}

export function nbody(steps) {
	const bodies = BODIES.map((values) => new Body(...values));
	offsetMomentum(bodies);
	const before = energy(bodies);
	advance(bodies, 0.01, steps);
	return `${before.toFixed(9)};${energy(bodies).toFixed(9)}`;
}
