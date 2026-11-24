// Race: nqueens (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

function nqueens(n) {
	const cols = new Array(n).fill(false);
	const up = new Array(2 * n - 1).fill(false);
	const down = new Array(2 * n - 1).fill(false);
	const place = (row) => {
		if (row === n) return 1;
		let count = 0;
		for (let col = 0; col < n; col++) {
			const u = row + col;
			const d = row + n - 1 - col;
			if (!cols[col] && !up[u] && !down[d]) {
				cols[col] = up[u] = down[d] = true;
				count += place(row + 1);
				cols[col] = up[u] = down[d] = false;
			}
		}
		return count;
	};
	return place(0);
}
