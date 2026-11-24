// Race: fannkuch (js)
// Extracted from suite/js/bg.mjs — build and run via suite/run.py

export function fannkuch(n) {
	const perm1 = Array.from({ length: n }, (_, i) => i);
	const count = new Array(n).fill(0);
	const perm = new Array(n).fill(0);
	let maxFlips = 0;
	let checksum = 0;
	let permCount = 0;
	let r = n;
	while (true) {
		while (r !== 1) {
			count[r - 1] = r;
			r--;
		}
		for (let i = 0; i < n; i++) perm[i] = perm1[i];
		let flips = 0;
		let k = perm[0];
		while (k !== 0) {
			for (let lo = 0, hi = k; lo < hi; lo++, hi--) {
				const t = perm[lo];
				perm[lo] = perm[hi];
				perm[hi] = t;
			}
			flips++;
			k = perm[0];
		}
		maxFlips = Math.max(maxFlips, flips);
		checksum += permCount % 2 === 0 ? flips : -flips;
		while (true) {
			if (r === n) return `${checksum}:${maxFlips}`;
			const perm0 = perm1[0];
			for (let i = 0; i < r; i++) perm1[i] = perm1[i + 1];
			perm1[r] = perm0;
			count[r]--;
			if (count[r] > 0) break;
			r++;
		}
		permCount++;
	}
}
