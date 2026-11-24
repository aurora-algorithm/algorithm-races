// Race: sudoku (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";

function shuffle(rng, items) {
	for (let i = items.length - 1; i > 0; i--) {
		const j = rng.next() % (i + 1);
		[items[i], items[j]] = [items[j], items[i]];
	}
}

const range = (n) => Array.from({ length: n }, (_, i) => i);

const SUDOKU_CLUES = 26;

const DLX_COLUMNS = 324;

function sudokuPuzzles(n) {
	const rng = new XorShift32();
	const puzzles = [];
	for (let p = 0; p < n; p++) {
		const digits = range(9).map((i) => i + 1);
		shuffle(rng, digits);
		const order = [];
		for (let axis = 0; axis < 2; axis++) {
			const bands = [0, 1, 2];
			shuffle(rng, bands);
			const lines = [];
			for (const band of bands) {
				const inner = [0, 1, 2];
				shuffle(rng, inner);
				for (const i of inner) lines.push(band * 3 + i);
			}
			order.push(lines);
		}
		const [rows, cols] = order;
		const cells = range(81);
		shuffle(rng, cells);
		const puzzle = new Array(81).fill(0);
		for (const cell of cells.slice(0, SUDOKU_CLUES)) {
			const r = rows[Math.floor(cell / 9)];
			const c = cols[cell % 9];
			puzzle[cell] = digits[(r * 3 + Math.floor(r / 3) + c) % 9];
		}
		puzzles.push(puzzle);
	}
	return puzzles;
}

// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
class Dlx {
	constructor() {
		const size = 1 + DLX_COLUMNS + 729 * 4;
		this.left = new Int32Array(size);
		this.right = new Int32Array(size);
		this.up = Int32Array.from(range(size));
		this.down = Int32Array.from(range(size));
		this.col = Int32Array.from(range(size));
		this.row = new Int32Array(size);
		this.count = new Int32Array(DLX_COLUMNS + 1);
		for (let i = 0; i <= DLX_COLUMNS; i++) {
			this.left[i] = i - 1;
			this.right[i] = i + 1;
		}
		this.left[0] = DLX_COLUMNS;
		this.right[DLX_COLUMNS] = 0;
		let node = DLX_COLUMNS + 1;
		for (let r = 0; r < 9; r++) {
			for (let c = 0; c < 9; c++) {
				const box = Math.floor(r / 3) * 3 + Math.floor(c / 3);
				for (let d = 0; d < 9; d++) {
					const first = node;
					const columns = [
						1 + r * 9 + c,
						82 + r * 9 + d,
						163 + c * 9 + d,
						244 + box * 9 + d,
					];
					for (const column of columns) {
						this.col[node] = column;
						this.row[node] = (r * 9 + c) * 9 + d;
						this.up[node] = this.up[column];
						this.down[node] = column;
						this.down[this.up[column]] = node;
						this.up[column] = node;
						this.count[column]++;
						this.left[node] = node - 1;
						this.right[node] = node + 1;
						node++;
					}
					this.left[first] = node - 1;
					this.right[node - 1] = first;
				}
			}
		}
		this.solutions = 0;
		this.tries = 0;
		this.chosen = new Int32Array(81);
		this.first = [];
	}

	cover(c) {
		const { left, right, up, down, col, count } = this;
		right[left[c]] = right[c];
		left[right[c]] = left[c];
		for (let i = down[c]; i !== c; i = down[i]) {
			for (let j = right[i]; j !== i; j = right[j]) {
				down[up[j]] = down[j];
				up[down[j]] = up[j];
				count[col[j]]--;
			}
		}
	}

	uncover(c) {
		const { left, right, up, down, col, count } = this;
		for (let i = up[c]; i !== c; i = up[i]) {
			for (let j = left[i]; j !== i; j = left[j]) {
				count[col[j]]++;
				down[up[j]] = j;
				up[down[j]] = j;
			}
		}
		right[left[c]] = c;
		left[right[c]] = c;
	}

	chooseRow(node) {
		for (let j = this.right[node]; j !== node; j = this.right[j]) {
			this.cover(this.col[j]);
		}
	}

	unchooseRow(node) {
		for (let j = this.left[node]; j !== node; j = this.left[j]) {
			this.uncover(this.col[j]);
		}
	}

	search(depth) {
		const { right, count } = this;
		if (right[0] === 0) {
			this.solutions++;
			if (this.solutions === 1)
				this.first = Array.from(this.chosen.subarray(0, depth));
			return;
		}
		let best = right[0];
		for (let c = right[best]; c !== 0; c = right[c]) {
			if (count[c] < count[best]) best = c;
		}
		this.cover(best);
		for (let r = this.down[best]; r !== best; r = this.down[r]) {
			this.tries++;
			this.chosen[depth] = this.row[r];
			this.chooseRow(r);
			this.search(depth + 1);
			this.unchooseRow(r);
		}
		this.uncover(best);
	}
}

function sudoku(puzzles) {
	let solutions = 0;
	let tries = 0;
	let h = 0;
	for (const puzzle of puzzles) {
		const dlx = new Dlx();
		let depth = 0;
		puzzle.forEach((digit, cell) => {
			if (digit === 0) return;
			const node = DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4;
			dlx.cover(dlx.col[node]);
			dlx.chooseRow(node);
			dlx.chosen[depth] = cell * 9 + digit - 1;
			depth++;
		});
		dlx.search(depth);
		solutions += dlx.solutions;
		tries += dlx.tries;
		const grid = new Array(81).fill(0);
		for (const row of dlx.first) grid[Math.floor(row / 9)] = (row % 9) + 1;
		h = h31([h, ...grid]);
	}
	return `${puzzles.length}:${solutions}:${tries}:${h}`;
}
