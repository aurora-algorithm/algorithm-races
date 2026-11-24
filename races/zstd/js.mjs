// Race: zstd (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { createHash } from "node:crypto";
import { createReadStream, readFileSync } from "node:fs";
import { join } from "node:path";
import zlib from "node:zlib";

const corpus = (n, dir) => readFileSync(join(dir, `corpus-${n}.bin`));

const plainChecksum = (raw) =>
	`${raw.length}:${createHash("sha256").update(raw).digest("hex")}`;

const zstd = (raw) =>
	zlib.zstdCompressSync(raw, {
		params: { [zlib.constants.ZSTD_c_compressionLevel]: 3 },
	});

const compressed = (decompress) => (packed) => ({
	checksum: plainChecksum(decompress(packed)),
	extra: { compressed_bytes: packed.length },
});
