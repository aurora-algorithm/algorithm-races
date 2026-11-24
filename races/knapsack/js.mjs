// Race: knapsack (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

const KNAPSACK_ITEMS = 1000;

function items(capacity) {
	const rng = new XorShift32();
	const things = [];
	for (let i = 0; i < KNAPSACK_ITEMS; i++) {
		const weight = 1 + (rng.next() % 1000);
		things.push({ weight, value: 1 + (rng.next() % 1000) });
	}
	return { capacity, things };
}

function knapsack({ capacity, things }) {
	const best = new Float64Array(capacity + 1);
	for (const { weight, value } of things) {
		for (let c = capacity; c >= weight; c--) {
			if (best[c - weight] + value > best[c])
				best[c] = best[c - weight] + value;
		}
	}
	return `${best[capacity]}:${h31(best)}`;
}
