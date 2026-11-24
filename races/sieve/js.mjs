// Race: sieve (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

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
