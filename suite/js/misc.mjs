// Library and loop kernels (kernels/SPEC.md section 5).

import { createHash } from "node:crypto";
import { fnv1a, h31, utf8 } from "./gen.mjs";

const LOG_PATTERN = /(GET|POST) \/api\/v[0-9]\/users\/([0-9]+) (5[0-9][0-9])/g;

export function regexLog(text) {
	let count = 0;
	let ids = 0;
	for (const m of text.matchAll(LOG_PATTERN)) {
		count++;
		ids += Number(m[2]);
	}
	return `${count}:${ids}`;
}

export function hashmap(inserts, lookups) {
	const counts = new Map();
	for (const k of inserts) counts.set(k, (counts.get(k) ?? 0) + 1);
	let hits = 0;
	for (const k of lookups) {
		if (counts.has(k)) hits++;
	}
	return `${counts.size}:${hits}`;
}

export const sha256 = (buf) => createHash("sha256").update(buf).digest("hex");

export function sieve(n) {
	const composite = new Uint8Array(n + 1);
	for (let i = 2; i * i <= n; i++) {
		if (!composite[i]) {
			for (let j = i * i; j <= n; j += i) composite[j] = 1;
		}
	}
	let count = 0;
	for (let i = 2; i <= n; i++) {
		if (!composite[i]) count++;
	}
	return String(count);
}

export function matmul({ a, b, n }) {
	const c = new Float64Array(n * n);
	for (let i = 0; i < n; i++) {
		const row = i * n;
		for (let j = 0; j < n; j++) {
			let s = 0;
			for (let k = 0; k < n; k++) s += a[row + k] * b[k * n + j];
			c[row + j] = s;
		}
	}
	return c;
}

export function matmulChecksum(c) {
	let total = 0;
	for (const v of c) total += v;
	return `${total}:${h31(c)}`;
}

export function jsonRoundtrip(text) {
	const doc = JSON.parse(text);
	let total = 0;
	let items = 0;
	for (const order of doc.orders) {
		total += order.total_cents;
		items += order.items;
	}
	return { count: doc.orders.length, total, items, out: JSON.stringify(doc) };
}

export function jsonChecksum({ count, total, items, out }) {
	const data = utf8(out);
	return `${count}:${total}:${items}:${data.length}:${fnv1a(data)}`;
}
