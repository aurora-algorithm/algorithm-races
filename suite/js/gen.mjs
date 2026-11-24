// Shared inputs and checksums (kernels/SPEC.md section 2).

export const SEED = 2463534242;
const FNV_OFFSET = 2166136261;
const FNV_PRIME = 16777619;

export class XorShift32 {
	constructor(seed = SEED) {
		this.s = seed;
	}

	next() {
		let s = this.s;
		s = (s ^ (s << 13)) >>> 0;
		s = (s ^ (s >>> 17)) >>> 0;
		s = (s ^ (s << 5)) >>> 0;
		this.s = s;
		return s;
	}

	ints(n) {
		const out = [];
		for (let i = 0; i < n; i++) {
			out.push(this.next());
		}
		return out;
	}
}

export function h31(values) {
	let h = 0;
	for (const v of values) {
		h = (Math.imul(h, 31) + v) >>> 0;
	}
	return h;
}

export function fnv1a(bytes) {
	let h = FNV_OFFSET;
	for (const b of bytes) {
		h = Math.imul(h ^ b, FNV_PRIME) >>> 0;
	}
	return h;
}

export const utf8 = (text) => new TextEncoder().encode(text);

export function randomStrings(n) {
	const rng = new XorShift32();
	const out = [];
	for (let i = 0; i < n; i++) {
		const length = 8 + (rng.next() % 9);
		let s = "";
		for (let c = 0; c < length; c++) {
			s += String.fromCharCode(97 + (rng.next() % 26));
		}
		out.push(s);
	}
	return out;
}

export function shaBuffer(size) {
	const oneMiB = 1 << 20;
	const block = new Uint8Array(oneMiB);
	const view = new DataView(block.buffer);
	new XorShift32().ints(oneMiB / 4).forEach((v, i) => {
		view.setUint32(i * 4, v, true);
	});
	const buf = new Uint8Array(size);
	for (let offset = 0; offset < size; offset += oneMiB) {
		buf.set(block, offset);
	}
	return buf;
}

const METHODS = ["GET", "POST", "PUT", "DELETE"];
const STATUSES = [200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503];

export function logText(lines) {
	const rng = new XorShift32();
	const parts = [];
	for (let i = 0; i < lines; i++) {
		const method = METHODS[rng.next() % 4];
		const version = 1 + (rng.next() % 3);
		const user = rng.next() % 100000;
		const status = STATUSES[rng.next() % 12];
		const ms = rng.next() % 1000;
		parts.push(`${method} /api/v${version}/users/${user} ${status} ${ms}ms\n`);
	}
	return parts.join("");
}

export function matrices(n) {
	const rng = new XorShift32();
	const a = Float64Array.from(rng.ints(n * n), (v) => v % 10);
	const b = Float64Array.from(rng.ints(n * n), (v) => v % 10);
	return { a, b, n };
}
