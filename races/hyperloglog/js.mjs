// Race: hyperloglog (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";

// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
function fmix32(key) {
	let h = key;
	h ^= h >>> 16;
	h = Math.imul(h, 0x85ebca6b);
	h ^= h >>> 13;
	h = Math.imul(h, 0xc2b2ae35);
	h ^= h >>> 16;
	return h >>> 0;
}

const HLL_P = 14;

const HLL_M = 1 << HLL_P;

function hyperloglog(n) {
	const rng = new XorShift32();
	const universe = Math.max(Math.floor(n / 4), 1);
	const registers = new Uint8Array(HLL_M);
	for (let i = 0; i < n; i++) {
		const h = fmix32(rng.next() % universe);
		const w = (h << HLL_P) >>> 0;
		const rank = w !== 0 ? Math.clz32(w) + 1 : 32 - HLL_P + 1;
		const idx = h >>> (32 - HLL_P);
		if (rank > registers[idx]) registers[idx] = rank;
	}
	return hllResult(n, registers);
}

function hllResult(n, registers) {
	let z = 0;
	let zeros = 0;
	for (const r of registers) {
		z += 2 ** -r;
		if (r === 0) zeros++;
	}
	const m = HLL_M;
	let estimate = ((0.7213 / (1 + 1.079 / m)) * m * m) / z;
	if (estimate <= 2.5 * m && zeros > 0) estimate = m * Math.log(m / zeros);
	return `${n}:${Math.floor(estimate + 0.5)}:${h31(registers)}`;
}
