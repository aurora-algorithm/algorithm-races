// Race: mergesort-mt (js)
// Extracted from suite/js/mt.mjs — build and run via suite/run.py

export const chunkBounds = (n, threads) =>
	Array.from({ length: threads + 1 }, (_, t) => Math.floor((t * n) / threads));

// `data` is a SharedArrayBuffer of Float64 values; the sorted result is left in it.
export async function mergesortMt(data, pool) {
	const aux = new SharedArrayBuffer(data.byteLength);
	let bounds = chunkBounds(
		data.byteLength / Float64Array.BYTES_PER_ELEMENT,
		pool.size,
	);
	await Promise.all(
		pool.workers.map((_, t) =>
			pool.run(t, "mergesort", { data, aux, lo: bounds[t], hi: bounds[t + 1] }),
		),
	);
	while (bounds.length > 2) {
		const merges = [];
		for (let p = 0; p + 2 < bounds.length; p += 2) {
			const args = {
				data,
				aux,
				lo: bounds[p],
				mid: bounds[p + 1],
				hi: bounds[p + 2],
			};
			merges.push(pool.run(p / 2, "merge", args));
		}
		await Promise.all(merges);
		bounds = bounds.filter((_, i) => i % 2 === 0 || i === bounds.length - 1);
	}
	return new Float64Array(data);
}
