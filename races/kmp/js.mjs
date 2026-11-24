// Race: kmp (js)
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

function kmp({ text: t, pattern: p }) {
	const m = p.length;
	const fail = new Int32Array(m);
	let k = 0;
	for (let i = 1; i < m; i++) {
		while (k > 0 && p.charCodeAt(i) !== p.charCodeAt(k)) k = fail[k - 1];
		if (p.charCodeAt(i) === p.charCodeAt(k)) k++;
		fail[i] = k;
	}
	let count = 0;
	let first = -1;
	let last = -1;
	k = 0;
	for (let i = 0; i < t.length; i++) {
		const ch = t.charCodeAt(i);
		while (k > 0 && ch !== p.charCodeAt(k)) k = fail[k - 1];
		if (ch === p.charCodeAt(k)) k++;
		if (k === m) {
			last = i + 1 - m;
			if (first < 0) first = last;
			count++;
			k = fail[k - 1];
		}
	}
	return `${count}:${first}:${last}`;
}
