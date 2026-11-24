# Race: json (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

import json
from gen import fnv1a, h31


def json_roundtrip(text: str) -> tuple[int, int, int, str]:
    doc = json.loads(text)
    total = 0
    items = 0
    for order in doc["orders"]:
        total += order["total_cents"]
        items += order["items"]
    out = json.dumps(doc, separators=(",", ":"))
    return len(doc["orders"]), total, items, out


def json_checksum(result: tuple[int, int, int, str]) -> str:
    count, total, items, out = result
    data = out.encode()
    return f"{count}:{total}:{items}:{len(data)}:{fnv1a(data)}"
