// Race: bwt (js)
// Extracted from suite/js/puzzle-text.mjs — build and run via suite/run.py

import { fnv1a, utf8, XorShift32 } from "./gen.mjs";

const DNA = utf8("ACGT");

// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
function dnaInto(rng, out) {
	for (let i = 0; i < out.length; i += 16) {
		let v = rng.next();
		for (let j = i; j < Math.min(i + 16, out.length); j++) {
			out[j] = DNA[v & 3];
			v >>>= 2;
		}
	}
	return out;
}

export const dnaText = (n) => dnaInto(new XorShift32(), new Uint8Array(n));

export function bwt(dna) {
	const n = dna.length + 1;
	const text = new Uint8Array(n);
	text.set(dna);
	text[n - 1] = "$".charCodeAt(0);
	let rank = Int32Array.from(text);
	const sa = Array.from({ length: n }, (_, i) => i);
	let k = 1;
	let rounds = 0;
	for (;;) {
		rounds++;
		const second = new Int32Array(n);
		for (let i = 0; i < n; i++) second[i] = i + k < n ? rank[i + k] : -1;
		const r = rank;
		sa.sort((a, b) => r[a] - r[b] || second[a] - second[b]);
		const tmp = new Int32Array(n);
		for (let j = 1; j < n; j++) {
			const a = sa[j - 1];
			const b = sa[j];
			const differs = r[a] !== r[b] || second[a] !== second[b];
			tmp[b] = tmp[a] + (differs ? 1 : 0);
		}
		rank = tmp;
		if (rank[sa[n - 1]] === n - 1) break;
		k *= 2;
	}
	const out = new Uint8Array(n);
	let primary = 0;
	sa.forEach((i, j) => {
		if (i === 0) {
			primary = j;
			out[j] = text[n - 1];
		} else {
			out[j] = text[i - 1];
		}
	});
	return { n, rounds, primary, out };
}

export const bwtChecksum = ({ n, rounds, primary, out }) =>
	`${n}:${rounds}:${primary}:${fnv1a(out)}`;
