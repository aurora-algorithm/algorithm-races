// Race: unzstd (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { readFileSync } from "node:fs";
import { join } from "node:path";
import zlib from "node:zlib";
import { fnv1a } from "./gen.mjs";

const corpus = (n, dir) => readFileSync(join(dir, `corpus-${n}.bin`));

const zstd = (raw) =>
	zlib.zstdCompressSync(raw, {
		params: { [zlib.constants.ZSTD_c_compressionLevel]: 3 },
	});

export const unzstd = (packed) => zlib.zstdDecompressSync(packed);

const plainChecksum = (raw) => `${raw.length}:${fnv1a(raw)}`;
