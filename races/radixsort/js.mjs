// Race: radixsort (js)
// Extracted from suite/js/sorts.mjs — build and run via suite/run.py

export function radixSort(a) {
	let src = a;
	let dst = new Array(a.length).fill(0);
	for (const shift of [0, 8, 16, 24]) {
		const count = new Array(256).fill(0);
		for (const v of src) count[(v >>> shift) & 255]++;
		const pos = new Array(256).fill(0);
		let total = 0;
		for (let d = 0; d < 256; d++) {
			pos[d] = total;
			total += count[d];
		}
		for (const v of src) dst[pos[(v >>> shift) & 255]++] = v;
		[src, dst] = [dst, src];
	}
	return src;
}
