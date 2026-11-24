// Race: palindrome (js)
// Extracted from suite/js/puzzle-text.mjs — build and run via suite/run.py

import { fnv1a, utf8, XorShift32 } from "./gen.mjs";

const CHAR_A = 97;

const CHAR_B = 98;

// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
export function binaryText(n) {
	const rng = new XorShift32();
	const out = new Uint8Array(n);
	for (let i = 0; i < n; i += 32) {
		let v = rng.next();
		for (let j = i; j < Math.min(i + 32, n); j++) {
			out[j] = v & 1 ? CHAR_B : CHAR_A;
			v >>>= 1;
		}
	}
	return out;
}

export function palindrome(text) {
	const t = new Uint8Array(2 * text.length + 3).fill("#".charCodeAt(0));
	t[0] = "^".charCodeAt(0);
	t[t.length - 1] = "$".charCodeAt(0);
	text.forEach((ch, i) => {
		t[2 + 2 * i] = ch;
	});
	const p = new Int32Array(t.length);
	let center = 0;
	let right = 0;
	for (let i = 1; i < t.length - 1; i++) {
		if (i < right) p[i] = Math.min(right - i, p[2 * center - i]);
		while (t[i + 1 + p[i]] === t[i - 1 - p[i]]) p[i]++;
		if (i + p[i] > right) {
			center = i;
			right = i + p[i];
		}
	}
	let longest = 0;
	let best = 0;
	let count = 0;
	p.forEach((radius, i) => {
		count += (radius + 1) >> 1;
		if (radius > longest) {
			longest = radius;
			best = i;
		}
	});
	return `${text.length}:${longest}:${Math.floor((best - 1 - longest) / 2)}:${count}`;
}
