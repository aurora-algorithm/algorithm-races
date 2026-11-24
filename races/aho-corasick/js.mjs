// Race: aho-corasick (js)
// Extracted from suite/js/puzzle-text.mjs — build and run via suite/run.py

import { fnv1a, utf8, XorShift32 } from "./gen.mjs";

const ONE_MIB = 1 << 20;

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
