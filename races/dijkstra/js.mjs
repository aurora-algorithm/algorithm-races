// Race: dijkstra (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
function graph(n) {
	const rng = new XorShift32();
	const to = new Int32Array(4 * n);
	const weight = new Int32Array(4 * n);
	for (let u = 0; u < n; u++) {
		to[4 * u] = (u + 1) % n;
		weight[4 * u] = 1 + (rng.next() % 100);
		for (let e = 4 * u + 1; e < 4 * u + 4; e++) {
			to[e] = rng.next() % n;
			weight[e] = 1 + (rng.next() % 100);
		}
	}
	return { n, to, weight };
}

// JavaScript has no standard priority queue: a binary min-heap of [dist, node] pairs.
class MinHeap {
	items = [];

	get size() {
		return this.items.length;
	}

	push(item) {
		const a = this.items;
		a.push(item);
		let i = a.length - 1;
		while (i > 0) {
			const parent = (i - 1) >> 1;
			if (a[parent][0] <= item[0]) break;
			a[i] = a[parent];
			i = parent;
		}
		a[i] = item;
	}

	pop() {
		const a = this.items;
		const top = a[0];
		const last = a.pop();
		if (a.length === 0) return top;
		let i = 0;
		for (;;) {
			let child = 2 * i + 1;
			if (child >= a.length) break;
			if (child + 1 < a.length && a[child + 1][0] < a[child][0]) child++;
			if (a[child][0] >= last[0]) break;
			a[i] = a[child];
			i = child;
		}
		a[i] = last;
		return top;
	}
}

function dijkstra({ n, to, weight }) {
	const dist = new Array(n).fill(Number.POSITIVE_INFINITY);
	dist[0] = 0;
	const heap = new MinHeap();
	heap.push([0, 0]);
	while (heap.size > 0) {
		const [d, u] = heap.pop();
		if (d > dist[u]) continue;
		for (let e = 4 * u; e < 4 * u + 4; e++) {
			const v = to[e];
			const nd = d + weight[e];
			if (nd < dist[v]) {
				dist[v] = nd;
				heap.push([nd, v]);
			}
		}
	}
	let reached = 0;
	let total = 0;
	let longest = 0;
	for (const d of dist) {
		if (d < Number.POSITIVE_INFINITY) {
			reached++;
			total += d;
			longest = Math.max(longest, d);
		}
	}
	return `${reached}:${total}:${longest}`;
}
