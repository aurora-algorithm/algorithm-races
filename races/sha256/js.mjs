// Race: sha256 (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

import { createHash } from "node:crypto";

export const sha256 = (buf) => createHash("sha256").update(buf).digest("hex");
