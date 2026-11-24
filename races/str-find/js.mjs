// Race: str-find (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

const PATTERN_LENGTH = 8;

const ACGT = [..."acgt"].map((c) => c.charCodeAt(0));

function dna(length, count) {
	const rng = new XorShift32();
	const out = [];
	for (let s = 0; s < count; s++) {
		const bytes = new Uint8Array(length);
		for (let i = 0; i < length; i++) bytes[i] = ACGT[rng.next() % 4];
		out.push(new TextDecoder("latin1").decode(bytes));
	}
	return out;
}

function text(n) {
	const t = dna(n, 1)[0];
	const mid = Math.floor(n / 2);
	return { text: t, pattern: t.slice(mid, mid + PATTERN_LENGTH) };
}

function strFind({ text: t, pattern: p }) {
	let count = 0;
	let first = -1;
	let last = -1;
	for (let pos = t.indexOf(p); pos >= 0; pos = t.indexOf(p, pos + 1)) {
		if (first < 0) first = pos;
		last = pos;
		count++;
	}
	return `${count}:${first}:${last}`;
}
