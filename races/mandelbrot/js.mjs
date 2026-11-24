// Race: mandelbrot (js)
// Extracted from suite/js/bg.mjs — build and run via suite/run.py

export const rowBytes = (n) => (n + 7) >> 3;

export function mandelbrotRow(n, y, out) {
	const row = y * rowBytes(n);
	const ci = (2.0 * y) / n - 1.0;
	for (let x = 0; x < n; x++) {
		const cr = (2.0 * x) / n - 1.5;
		let zr = 0;
		let zi = 0;
		let tr = 0;
		let ti = 0;
		let i = 0;
		while (i < 50 && tr + ti <= 4.0) {
			zi = 2.0 * zr * zi + ci;
			zr = tr - ti + cr;
			tr = zr * zr;
			ti = zi * zi;
			i++;
		}
		if (tr + ti <= 4.0) out[row + (x >> 3)] |= 0x80 >> (x & 7);
	}
}

export function mandelbrot(n) {
	const out = new Uint8Array(n * rowBytes(n));
	for (let y = 0; y < n; y++) mandelbrotRow(n, y, out);
	return out;
}
