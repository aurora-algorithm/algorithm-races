// Race: lis (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

// JavaScript has no standard binary search: the usual lower bound.
function lowerBound(a, x) {
	let lo = 0;
	let hi = a.length;
	while (lo < hi) {
		const mid = (lo + hi) >>> 1;
		if (a[mid] < x) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

function lis(values) {
	const tails = [];
	for (const x of values) {
		const i = lowerBound(tails, x);
		if (i === tails.length) tails.push(x);
		else tails[i] = x;
	}
	return `${tails.length}:${h31(tails)}`;
}
