// Race: life (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";

const LIFE_SIDE = 512;

function lifeInput(gens) {
	const rng = new XorShift32();
	const grid = Uint8Array.from({ length: LIFE_SIDE * LIFE_SIDE }, () =>
		rng.next() % 4 === 0 ? 1 : 0,
	);
	return { grid, gens };
}

function life({ grid, gens }) {
	const w = LIFE_SIDE;
	let cur = grid.slice();
	let next = new Uint8Array(grid.length);
	for (let g = 0; g < gens; g++) {
		for (let y = 0; y < w; y++) {
			const up = ((y + w - 1) % w) * w;
			const row = y * w;
			const down = ((y + 1) % w) * w;
			for (let x = 0; x < w; x++) {
				const left = (x + w - 1) % w;
				const right = (x + 1) % w;
				const n =
					cur[up + left] +
					cur[up + x] +
					cur[up + right] +
					cur[row + left] +
					cur[row + right] +
					cur[down + left] +
					cur[down + x] +
					cur[down + right];
				next[row + x] = n === 3 || (n === 2 && cur[row + x] === 1) ? 1 : 0;
			}
		}
		[cur, next] = [next, cur];
	}
	return { gens, grid: cur };
}

function lifeChecksum({ gens, grid }) {
	let alive = 0;
	for (const cell of grid) alive += cell;
	return `${gens}:${alive}:${fnv1a(grid)}`;
}
