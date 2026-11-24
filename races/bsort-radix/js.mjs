// Race: bsort-int.go.zermelo.Sort (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

const ints = (n) => new XorShift32().ints(n);
