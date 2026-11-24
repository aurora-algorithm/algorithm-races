// Race: regex-log (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

const LOG_PATTERN = /(GET|POST) \/api\/v[0-9]\/users\/([0-9]+) (5[0-9][0-9])/g;

export function regexLog(text) {
	let count = 0;
	let ids = 0;
	for (const m of text.matchAll(LOG_PATTERN)) {
		count++;
		ids += Number(m[2]);
	}
	return `${count}:${ids}`;
}
