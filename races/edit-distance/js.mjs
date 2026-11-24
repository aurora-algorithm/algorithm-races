// Race: edit-distance (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

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

function editDistance([a, b]) {
	let prev = new Int32Array(b.length + 1);
	let cur = new Int32Array(b.length + 1);
	for (let j = 0; j <= b.length; j++) prev[j] = j;
	for (let i = 0; i < a.length; i++) {
		cur[0] = i + 1;
		const ca = a.charCodeAt(i);
		for (let j = 0; j < b.length; j++) {
			const cost = ca === b.charCodeAt(j) ? 0 : 1;
			cur[j + 1] = Math.min(prev[j + 1] + 1, cur[j] + 1, prev[j] + cost);
		}
		[prev, cur] = [cur, prev];
	}
	return String(prev[b.length]);
}
