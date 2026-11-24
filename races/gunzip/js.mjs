// Race: gunzip (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { readFileSync } from "node:fs";
import { join } from "node:path";
import zlib from "node:zlib";
import { fnv1a } from "./gen.mjs";

const corpus = (n, dir) => readFileSync(join(dir, `corpus-${n}.bin`));

const gzip = (raw) => zlib.gzipSync(raw, { level: 6 });

export const gunzip = (packed) => zlib.gunzipSync(packed);

const plainChecksum = (raw) => `${raw.length}:${fnv1a(raw)}`;
