// Race: kruskal (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

function edgeList(n) {
	const rng = new XorShift32();
	const edges = [];
	for (let i = 0; i < 4 * n; i++) {
		const u = rng.next() % n;
		const v = rng.next() % n;
		edges.push({ w: rng.next() % 1000000, u, v });
	}
	return { n, edges };
}

function find(parent, x) {
	let root = x;
	while (parent[root] !== root) root = parent[root];
	while (parent[x] !== root) {
		const next = parent[x];
		parent[x] = root;
		x = next;
	}
	return root;
}

function kruskal({ n, edges }) {
	edges.sort((a, b) => a.w - b.w);
	const parent = new Int32Array(n);
	for (let i = 0; i < n; i++) parent[i] = i;
	const rank = new Uint8Array(n);
	let count = 0;
	let total = 0;
	for (const e of edges) {
		let ru = find(parent, e.u);
		let rv = find(parent, e.v);
		if (ru === rv) continue;
		if (rank[ru] < rank[rv]) [ru, rv] = [rv, ru];
		parent[rv] = ru;
		if (rank[ru] === rank[rv]) rank[ru]++;
		count++;
		total += e.w;
	}
	return `${count}:${total}`;
}

const copyEdges = ({ n, edges }) => ({ n, edges: edges.slice() });
