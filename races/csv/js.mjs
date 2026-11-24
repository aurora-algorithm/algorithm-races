// Race: csv (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { createReadStream, readFileSync } from "node:fs";
import { parse } from "csv-parse";

function addTotal(totals, customer, cents) {
	totals.set(customer, (totals.get(customer) ?? 0) + cents);
}

async function csvParse(path) {
	const totals = new Map();
	let rows = 0;
	const records = createReadStream(path).pipe(parse({ columns: true }));
	for await (const order of records) {
		rows++;
		addTotal(totals, Number(order.customer_id), Number(order.total_cents));
	}
	return { rows, totals };
}

function top(map, compareKeys) {
	let best;
	for (const entry of map) {
		if (
			best === undefined ||
			entry[1] > best[1] ||
			(entry[1] === best[1] && compareKeys(entry[0], best[0]) < 0)
		)
			best = entry;
	}
	return best;
}

function csvChecksum({ rows, totals }) {
	let sum = 0;
	for (const v of totals.values()) sum += v;
	const [id, total] = top(totals, (a, b) => a - b);
	return `${rows}:${totals.size}:${sum}:${id}:${total}`;
}
