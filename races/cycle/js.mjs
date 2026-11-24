// Race: cycle (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";

const CYCLE_MOD = 67108859;

const CYCLE_C = 12345;

function cycleStarts(n) {
	const rng = new XorShift32();
	return Array.from({ length: n }, () => rng.next() % CYCLE_MOD);
}

const step = (x) => (x * x + CYCLE_C) % CYCLE_MOD;

function cycle(starts) {
	let totalMu = 0;
	let totalLam = 0;
	let longest = 0;
	for (const x0 of starts) {
		let tortoise = step(x0);
		let hare = step(tortoise);
		while (tortoise !== hare) {
			tortoise = step(tortoise);
			hare = step(step(hare));
		}
		let mu = 0;
		tortoise = x0;
		while (tortoise !== hare) {
			tortoise = step(tortoise);
			hare = step(hare);
			mu++;
		}
		let lam = 1;
		hare = step(tortoise);
		while (tortoise !== hare) {
			hare = step(hare);
			lam++;
		}
		totalMu += mu;
		totalLam += lam;
		longest = Math.max(longest, mu + lam);
	}
	return `${starts.length}:${totalMu}:${totalLam}:${longest}`;
}
