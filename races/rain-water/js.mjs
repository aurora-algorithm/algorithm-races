// Race: rain-water (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";
import {
	ahoCorasick,
	ahoInput,
	binaryText,
	bwt,
	bwtChecksum,
	dnaText,
	huffman,
	huffmanChecksum,
	MinHeap,
	palindrome,
	skewedText,
} from "./puzzle-text.mjs";

function terrain(side) {
	const rng = new XorShift32();
	return Array.from({ length: side * side }, () => rng.next() % 1000);
}

const RAIN_DR = [-1, 1, 0, 0];

const RAIN_DC = [0, 0, -1, 1];

function rainWater(heights) {
	const n = heights.length;
	const side = Math.round(Math.sqrt(n));
	const seen = new Uint8Array(n);
	const heap = new MinHeap();
	for (let idx = 0; idx < n; idx++) {
		const r = Math.floor(idx / side);
		const c = idx % side;
		if (r === 0 || r === side - 1 || c === 0 || c === side - 1) {
			seen[idx] = 1;
			heap.push(heights[idx] * n + idx);
		}
	}
	let water = 0;
	let wet = 0;
	while (heap.size > 0) {
		const key = heap.pop();
		const level = Math.floor(key / n);
		const idx = key % n;
		const r = Math.floor(idx / side);
		const c = idx % side;
		for (let d = 0; d < 4; d++) {
			const nr = r + RAIN_DR[d];
			const nc = c + RAIN_DC[d];
			if (nr < 0 || nr >= side || nc < 0 || nc >= side) continue;
			const nidx = nr * side + nc;
			if (seen[nidx] === 1) continue;
			seen[nidx] = 1;
			let h = heights[nidx];
			if (h < level) {
				water += level - h;
				wet++;
				h = level;
			}
			heap.push(h * n + nidx);
		}
	}
	return `${side}:${water}:${wet}`;
}
