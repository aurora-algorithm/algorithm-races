// Race: binary-search (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

function sortedArray(n) {
	const rng = new XorShift32();
	const a = new Array(n);
	let v = 0;
	for (let i = 0; i < n; i++) {
		v += 1 + (rng.next() % 4);
		a[i] = v;
	}
	const queries = new Array(n);
	for (let i = 0; i < n; i++) queries[i] = rng.next() % (v + 1);
	return { a, queries };
}

function binarySearch({ a, queries }) {
	const n = a.length;
	let hits = 0;
	let total = 0;
	for (const q of queries) {
		let lo = 0;
		let hi = n;
		while (lo < hi) {
			const mid = (lo + hi) >>> 1;
			if (a[mid] < q) lo = mid + 1;
			else hi = mid;
		}
		if (lo < n && a[lo] === q) hits++;
		total += lo;
	}
	return `${hits}:${total}`;
}
