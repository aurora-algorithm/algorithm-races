// Benchmarks Game kernels, single-threaded (kernels/SPEC.md section 4). Floating-point operations keep the
// order written in the spec so the printed results match every other language to the last digit.

export const MIN_DEPTH = 4;

class TreeNode {
	constructor(left, right) {
		this.left = left;
		this.right = right;
	}
}

export function makeTree(depth) {
	if (depth === 0) return new TreeNode(null, null);
	return new TreeNode(makeTree(depth - 1), makeTree(depth - 1));
}

export function checkTree(node) {
	if (node.left === null) return 1;
	return 1 + checkTree(node.left) + checkTree(node.right);
}

export function depthChecks(depth, iterations) {
	let check = 0;
	for (let i = 0; i < iterations; i++) check += checkTree(makeTree(depth));
	return check;
}

export const maxDepthFor = (n) => Math.max(MIN_DEPTH + 2, n);
export const iterationsFor = (maxDepth, depth) =>
	1 << (maxDepth - depth + MIN_DEPTH);

export const stretchLine = (depth) =>
	`stretch tree of depth ${depth}\t check: ${checkTree(makeTree(depth))}`;
export const depthLine = (iterations, depth, check) =>
	`${iterations}\t trees of depth ${depth}\t check: ${check}`;
export const longLivedLine = (depth, tree) =>
	`long lived tree of depth ${depth}\t check: ${checkTree(tree)}`;

export function binaryTrees(n) {
	const maxDepth = maxDepthFor(n);
	const lines = [stretchLine(maxDepth + 1)];
	const longLived = makeTree(maxDepth);
	for (let depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
		const iterations = iterationsFor(maxDepth, depth);
		lines.push(depthLine(iterations, depth, depthChecks(depth, iterations)));
	}
	lines.push(longLivedLine(maxDepth, longLived));
	return `${lines.join("\n")}\n`;
}

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

export const rowBytes = (n) => (n + 7) >> 3;

export function mandelbrotRow(n, y, out) {
	const row = y * rowBytes(n);
	const ci = (2.0 * y) / n - 1.0;
	for (let x = 0; x < n; x++) {
		const cr = (2.0 * x) / n - 1.5;
		let zr = 0;
		let zi = 0;
		let tr = 0;
		let ti = 0;
		let i = 0;
		while (i < 50 && tr + ti <= 4.0) {
			zi = 2.0 * zr * zi + ci;
			zr = tr - ti + cr;
			tr = zr * zr;
			ti = zi * zi;
			i++;
		}
		if (tr + ti <= 4.0) out[row + (x >> 3)] |= 0x80 >> (x & 7);
	}
}

export function mandelbrot(n) {
	const out = new Uint8Array(n * rowBytes(n));
	for (let y = 0; y < n; y++) mandelbrotRow(n, y, out);
	return out;
}

export function fannkuch(n) {
	const perm1 = Array.from({ length: n }, (_, i) => i);
	const count = new Array(n).fill(0);
	const perm = new Array(n).fill(0);
	let maxFlips = 0;
	let checksum = 0;
	let permCount = 0;
	let r = n;
	while (true) {
		while (r !== 1) {
			count[r - 1] = r;
			r--;
		}
		for (let i = 0; i < n; i++) perm[i] = perm1[i];
		let flips = 0;
		let k = perm[0];
		while (k !== 0) {
			for (let lo = 0, hi = k; lo < hi; lo++, hi--) {
				const t = perm[lo];
				perm[lo] = perm[hi];
				perm[hi] = t;
			}
			flips++;
			k = perm[0];
		}
		maxFlips = Math.max(maxFlips, flips);
		checksum += permCount % 2 === 0 ? flips : -flips;
		while (true) {
			if (r === n) return `${checksum}:${maxFlips}`;
			const perm0 = perm1[0];
			for (let i = 0; i < r; i++) perm1[i] = perm1[i + 1];
			perm1[r] = perm0;
			count[r]--;
			if (count[r] > 0) break;
			r++;
		}
		permCount++;
	}
}

const evalA = (i, j) => 1.0 / (((i + j) * (i + j + 1)) / 2 + i + 1);

function mulAv(u, n) {
	const out = new Float64Array(n);
	for (let i = 0; i < n; i++) {
		let s = 0;
		for (let j = 0; j < n; j++) s += evalA(i, j) * u[j];
		out[i] = s;
	}
	return out;
}

function mulAtv(u, n) {
	const out = new Float64Array(n);
	for (let i = 0; i < n; i++) {
		let s = 0;
		for (let j = 0; j < n; j++) s += evalA(j, i) * u[j];
		out[i] = s;
	}
	return out;
}

const mulAtAv = (u, n) => mulAtv(mulAv(u, n), n);

export function spectralNorm(n) {
	let u = new Float64Array(n).fill(1);
	let v = u;
	for (let i = 0; i < 10; i++) {
		v = mulAtAv(u, n);
		u = mulAtAv(v, n);
	}
	let vBv = 0;
	let vv = 0;
	for (let i = 0; i < n; i++) {
		vBv += u[i] * v[i];
		vv += v[i] * v[i];
	}
	return Math.sqrt(vBv / vv).toFixed(9);
}
