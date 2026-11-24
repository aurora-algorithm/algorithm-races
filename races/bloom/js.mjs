// Race: bloom (js)
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

const BLOOM_BITS_PER_KEY = 10;

const BLOOM_HASHES = 7;

const GOLDEN = 0x9e3779b9;

function bloomInput(n) {
	const values = new XorShift32().ints(2 * n);
	return { keys: values.slice(0, n), lookups: values.slice(n) };
}

// SWAR popcount of one byte, as the other languages' popcount instructions count it.
function popcount8(byte) {
	const b2 = byte - ((byte >> 1) & 0x55);
	const b4 = (b2 & 0x33) + ((b2 >> 2) & 0x33);
	return (b4 + (b4 >> 4)) & 0x0f;
}

function bloom({ keys, lookups }) {
	const m = BLOOM_BITS_PER_KEY * keys.length;
	const bits = new Uint8Array(Math.ceil(m / 8));
	for (const key of keys) {
		const h1 = fmix32(key);
		const h2 = (fmix32((key ^ GOLDEN) >>> 0) | 1) >>> 0;
		for (let i = 0; i < BLOOM_HASHES; i++) {
			const b = ((h1 + i * h2) >>> 0) % m;
			bits[b >> 3] |= 1 << (b & 7);
		}
	}
	let falsePositives = 0;
	for (const key of lookups) {
		const h1 = fmix32(key);
		const h2 = (fmix32((key ^ GOLDEN) >>> 0) | 1) >>> 0;
		let all = true;
		for (let i = 0; i < BLOOM_HASHES; i++) {
			const b = ((h1 + i * h2) >>> 0) % m;
			if ((bits[b >> 3] & (1 << (b & 7))) === 0) {
				all = false;
				break;
			}
		}
		if (all) falsePositives++;
	}
	let setBits = 0;
	for (const byte of bits) setBits += popcount8(byte);
	return `${keys.length}:${setBits}:${falsePositives}`;
}
