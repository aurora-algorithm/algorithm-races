// The string kernels of the puzzle group (kernels/SPEC.md section 13): palindrome, bwt, aho-corasick, huffman.

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

// palindrome: Manacher's mirror trick finds every palindrome in linear time

const CHAR_A = 97;
const CHAR_B = 98;

// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
export function binaryText(n) {
	const rng = new XorShift32();
	const out = new Uint8Array(n);
	for (let i = 0; i < n; i += 32) {
		let v = rng.next();
		for (let j = i; j < Math.min(i + 32, n); j++) {
			out[j] = v & 1 ? CHAR_B : CHAR_A;
			v >>>= 1;
		}
	}
	return out;
}

export function palindrome(text) {
	const t = new Uint8Array(2 * text.length + 3).fill("#".charCodeAt(0));
	t[0] = "^".charCodeAt(0);
	t[t.length - 1] = "$".charCodeAt(0);
	text.forEach((ch, i) => {
		t[2 + 2 * i] = ch;
	});
	const p = new Int32Array(t.length);
	let center = 0;
	let right = 0;
	for (let i = 1; i < t.length - 1; i++) {
		if (i < right) p[i] = Math.min(right - i, p[2 * center - i]);
		while (t[i + 1 + p[i]] === t[i - 1 - p[i]]) p[i]++;
		if (i + p[i] > right) {
			center = i;
			right = i + p[i];
		}
	}
	let longest = 0;
	let best = 0;
	let count = 0;
	p.forEach((radius, i) => {
		count += (radius + 1) >> 1;
		if (radius > longest) {
			longest = radius;
			best = i;
		}
	});
	return `${text.length}:${longest}:${Math.floor((best - 1 - longest) / 2)}:${count}`;
}

// bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

const DNA = utf8("ACGT");

// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
function dnaInto(rng, out) {
	for (let i = 0; i < out.length; i += 16) {
		let v = rng.next();
		for (let j = i; j < Math.min(i + 16, out.length); j++) {
			out[j] = DNA[v & 3];
			v >>>= 2;
		}
	}
	return out;
}

export const dnaText = (n) => dnaInto(new XorShift32(), new Uint8Array(n));

export function bwt(dna) {
	const n = dna.length + 1;
	const text = new Uint8Array(n);
	text.set(dna);
	text[n - 1] = "$".charCodeAt(0);
	let rank = Int32Array.from(text);
	const sa = Array.from({ length: n }, (_, i) => i);
	let k = 1;
	let rounds = 0;
	for (;;) {
		rounds++;
		const second = new Int32Array(n);
		for (let i = 0; i < n; i++) second[i] = i + k < n ? rank[i + k] : -1;
		const r = rank;
		sa.sort((a, b) => r[a] - r[b] || second[a] - second[b]);
		const tmp = new Int32Array(n);
		for (let j = 1; j < n; j++) {
			const a = sa[j - 1];
			const b = sa[j];
			const differs = r[a] !== r[b] || second[a] !== second[b];
			tmp[b] = tmp[a] + (differs ? 1 : 0);
		}
		rank = tmp;
		if (rank[sa[n - 1]] === n - 1) break;
		k *= 2;
	}
	const out = new Uint8Array(n);
	let primary = 0;
	sa.forEach((i, j) => {
		if (i === 0) {
			primary = j;
			out[j] = text[n - 1];
		} else {
			out[j] = text[i - 1];
		}
	});
	return { n, rounds, primary, out };
}

export const bwtChecksum = ({ n, rounds, primary, out }) =>
	`${n}:${rounds}:${primary}:${fnv1a(out)}`;

// aho-corasick: a trie with failure links matches thousands of patterns in one pass

const AHO_PATTERNS = 10000;

export function ahoInput(n) {
	const rng = new XorShift32();
	const patterns = [];
	for (let p = 0; p < AHO_PATTERNS; p++) {
		const length = 6 + (rng.next() % 7);
		patterns.push(Uint8Array.from({ length }, () => DNA[rng.next() % 4]));
	}
	const block = dnaInto(rng, new Uint8Array(ONE_MIB));
	const text = new Uint8Array(n);
	for (let offset = 0; offset < n; offset += ONE_MIB) text.set(block, offset);
	return { patterns, text };
}

export function ahoCorasick({ patterns, text }) {
	const code = new Uint8Array(256);
	DNA.forEach((ch, i) => {
		code[ch] = i;
	});
	const goto = [-1, -1, -1, -1];
	const out = [0];
	for (const pattern of patterns) {
		let s = 0;
		for (const ch of pattern) {
			const edge = s * 4 + code[ch];
			if (goto[edge] === -1) {
				goto[edge] = out.length;
				goto.push(-1, -1, -1, -1);
				out.push(0);
			}
			s = goto[edge];
		}
		out[s]++;
	}
	const fail = new Array(out.length).fill(0);
	const queue = [];
	for (let c = 0; c < 4; c++) {
		if (goto[c] === -1) goto[c] = 0;
		else queue.push(goto[c]);
	}
	for (let head = 0; head < queue.length; head++) {
		const s = queue[head];
		out[s] += out[fail[s]];
		for (let c = 0; c < 4; c++) {
			const t = goto[s * 4 + c];
			if (t === -1) {
				goto[s * 4 + c] = goto[fail[s] * 4 + c];
			} else {
				fail[t] = goto[fail[s] * 4 + c];
				queue.push(t);
			}
		}
	}
	let s = 0;
	let matches = 0;
	let hits = 0;
	for (const ch of text) {
		s = goto[s * 4 + code[ch]];
		const found = out[s];
		if (found > 0) {
			matches += found;
			hits++;
		}
	}
	return `${patterns.length}:${out.length}:${matches}:${hits}`;
}

// huffman: merge the two rarest symbols until one tree is left

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
