// Race: spectral-norm (js)
// Extracted from suite/js/bg.mjs — build and run via suite/run.py

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
