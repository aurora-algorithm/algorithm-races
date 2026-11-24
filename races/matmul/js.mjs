// Race: matmul (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

import { fnv1a, h31, utf8 } from "./gen.mjs";

export function matmul({ a, b, n }) {
	const c = new Float64Array(n * n);
	for (let i = 0; i < n; i++) {
		const row = i * n;
		for (let j = 0; j < n; j++) {
			let s = 0;
			for (let k = 0; k < n; k++) s += a[row + k] * b[k * n + j];
			c[row + j] = s;
		}
	}
	return c;
}

export function matmulChecksum(c) {
	let total = 0;
	for (const v of c) total += v;
	return `${total}:${h31(c)}`;
}
