// Race: pi-digits (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

const CHUD_A = 13591409n;

const CHUD_B = 545140134n;

const CHUD_C3_24 = 640320n ** 3n / 24n;

const DIGITS_PER_TERM = 14;

const GUARD_DIGITS = 10n;

// P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
function binarySplit(a, b) {
	if (b - a === 1) {
		const k = BigInt(a);
		const p = a === 0 ? 1n : (6n * k - 5n) * (2n * k - 1n) * (6n * k - 1n);
		const q = a === 0 ? 1n : k * k * k * CHUD_C3_24;
		const t = p * (CHUD_A + CHUD_B * k);
		return [p, q, a % 2 === 1 ? -t : t];
	}
	const m = Math.floor((a + b) / 2);
	const [p1, q1, t1] = binarySplit(a, m);
	const [p2, q2, t2] = binarySplit(m, b);
	return [p1 * p2, q1 * q2, t1 * q2 + p1 * t2];
}

// Floor square root of n > 0 by precision doubling (CPython's math.isqrt): each step doubles the correct bits.
function isqrt(n) {
	const c = Math.floor((n.toString(2).length - 1) / 2);
	let a = 1n;
	let d = 0;
	for (let s = 31 - Math.clz32(c); s >= 0; s--) {
		const e = d;
		d = c >>> s;
		a = (a << BigInt(d - e - 1)) + (n >> BigInt(2 * c - e - d + 1)) / a;
	}
	return a * a > n ? a - 1n : a;
}

function piDigits(n) {
	const terms = Math.floor(n / DIGITS_PER_TERM) + 2;
	const [, q, t] = binarySplit(0, terms);
	const one = 10n ** (BigInt(n) + GUARD_DIGITS);
	const sqrtC = isqrt(10005n * one * one);
	const pi = (426880n * sqrtC * q) / t;
	return (pi / 10n ** GUARD_DIGITS).toString();
}

function piChecksum(digits) {
	let sum = 0;
	for (const ch of digits) sum += Number(ch);
	return `${digits.length - 1}:${digits.slice(-50)}:${sum}`;
}
