// Race: bfs-grid (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

function grid(n) {
	const rng = new XorShift32();
	const walls = new Uint8Array(n * n);
	for (let i = 0; i < walls.length; i++) walls[i] = rng.next() % 100 < 30;
	for (const cell of [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1]) {
		walls[cell] = 0;
	}
	return { n, walls };
}

function bfsGrid({ n, walls }) {
	const dist = new Int32Array(n * n).fill(-1);
	dist[0] = 0;
	const queue = [0];
	const visit = (next, ok, d) => {
		if (ok && !walls[next] && dist[next] < 0) {
			dist[next] = d;
			queue.push(next);
		}
	};
	for (let head = 0; head < queue.length; head++) {
		const cell = queue[head];
		const d = dist[cell] + 1;
		const row = Math.floor(cell / n);
		const col = cell % n;
		visit(cell - n, row > 0, d);
		visit(cell + n, row < n - 1, d);
		visit(cell - 1, col > 0, d);
		visit(cell + 1, col < n - 1, d);
	}
	let reached = 0;
	let total = 0;
	for (const d of dist) {
		if (d >= 0) {
			reached++;
			total += d;
		}
	}
	return `${reached}:${dist[n * n - 1]}:${total}`;
}
