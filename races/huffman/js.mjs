// Race: huffman (js)
// Extracted from suite/js/puzzle-text.mjs — build and run via suite/run.py

import { fnv1a, utf8, XorShift32 } from "./gen.mjs";

const ONE_MIB = 1 << 20;

// JavaScript has no priority queue: a binary min-heap over numbers.
export class MinHeap {
	constructor() {
		this.items = [];
	}

	get size() {
		return this.items.length;
	}

	push(value) {
		const a = this.items;
		let i = a.length;
		a.push(value);
		while (i > 0) {
			const parent = (i - 1) >> 1;
			if (a[parent] <= value) break;
			a[i] = a[parent];
			i = parent;
		}
		a[i] = value;
	}

	pop() {
		const a = this.items;
		const top = a[0];
		const last = a.pop();
		if (a.length > 0) {
			let i = 0;
			for (;;) {
				let child = 2 * i + 1;
				if (child >= a.length) break;
				if (child + 1 < a.length && a[child + 1] < a[child]) child++;
				if (a[child] >= last) break;
				a[i] = a[child];
				i = child;
			}
			a[i] = last;
		}
		return top;
	}
}

const CHAR_A = 97;

const HUFF_ID = 1024;

// A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
export function skewedText(n) {
	const rng = new XorShift32();
	const block = Uint8Array.from({ length: ONE_MIB }, () => {
		const a = rng.next() % 26;
		const b = rng.next() % 26;
		return CHAR_A + Math.floor((a * b) / 26);
	});
	const text = new Uint8Array(n);
	for (let offset = 0; offset < n; offset += ONE_MIB) text.set(block, offset);
	return text;
}

function huffmanTree(freq) {
	const heap = new MinHeap();
	freq.forEach((f, sym) => {
		if (f > 0) heap.push(f * HUFF_ID + sym);
	});
	const left = new Int32Array(512);
	const right = new Int32Array(512);
	let nextId = 256;
	while (heap.size > 1) {
		const a = heap.pop();
		const b = heap.pop();
		left[nextId] = a % HUFF_ID;
		right[nextId] = b % HUFF_ID;
		const weight = Math.floor(a / HUFF_ID) + Math.floor(b / HUFF_ID);
		heap.push(weight * HUFF_ID + nextId);
		nextId++;
	}
	return { root: heap.pop() % HUFF_ID, left, right };
}

function huffmanCodes({ root, left, right }) {
	const codes = new Int32Array(256);
	const lengths = new Int32Array(256);
	const stack = [[root, 0, 0]];
	while (stack.length > 0) {
		const [node, code, length] = stack.pop();
		if (node < 256) {
			codes[node] = code;
			lengths[node] = length;
		} else {
			stack.push([left[node], code << 1, length + 1]);
			stack.push([right[node], (code << 1) | 1, length + 1]);
		}
	}
	return { codes, lengths };
}

export function huffman(text) {
	const freq = new Array(256).fill(0);
	for (const b of text) freq[b]++;
	const tree = huffmanTree(freq);
	const { codes, lengths } = huffmanCodes(tree);
	const maxLength = Math.max(...lengths);
	const buffer = new Uint8Array(Math.ceil((text.length * maxLength) / 8) + 1);
	let size = 0;
	let acc = 0;
	let nbits = 0;
	let bits = 0;
	for (const b of text) {
		acc = (acc << lengths[b]) | codes[b];
		nbits += lengths[b];
		bits += lengths[b];
		while (nbits >= 8) {
			nbits -= 8;
			buffer[size++] = (acc >> nbits) & 0xff;
		}
		acc &= (1 << nbits) - 1;
	}
	if (nbits > 0) buffer[size++] = (acc << (8 - nbits)) & 0xff;
	const encoded = buffer.subarray(0, size);
	const { root, left, right } = tree;
	const decoded = new Uint8Array(text.length);
	let node = root;
	let k = 0;
	for (const byte of encoded) {
		for (let shift = 7; shift >= 0; shift--) {
			node = (byte >> shift) & 1 ? right[node] : left[node];
			if (node < 256) {
				decoded[k++] = node;
				node = root;
				if (k === text.length) break;
			}
		}
	}
	const ok = decoded.every((b, i) => b === text[i]);
	return { n: text.length, bits, encoded, ok };
}

export const huffmanChecksum = ({ n, bits, encoded, ok }) =>
	`${n}:${bits}:${fnv1a(encoded)}:${ok ? 1 : 0}`;
