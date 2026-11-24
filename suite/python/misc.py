"""Library and loop kernels (kernels/SPEC.md section 5)."""

import hashlib
import json
import re

from gen import fnv1a, h31

LOG_PATTERN = re.compile(r"(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])")


def regex_log(text: str) -> str:
    count = 0
    ids = 0
    for m in LOG_PATTERN.finditer(text):
        count += 1
        ids += int(m.group(2))
    return f"{count}:{ids}"


def hashmap(inserts: list[int], lookups: list[int]) -> str:
    counts: dict[int, int] = {}
    for k in inserts:
        counts[k] = counts.get(k, 0) + 1
    hits = 0
    for k in lookups:
        if k in counts:
            hits += 1
    return f"{len(counts)}:{hits}"


def sha256(buf: bytes) -> str:
    return hashlib.sha256(buf).hexdigest()


def sieve(n: int) -> str:
    composite = bytearray(n + 1)
    i = 2
    while i * i <= n:
        if not composite[i]:
            for j in range(i * i, n + 1, i):
                composite[j] = 1
        i += 1
    count = 0
    for i in range(2, n + 1):
        if not composite[i]:
            count += 1
    return str(count)


def matmul(a: list[float], b: list[float], n: int) -> list[float]:
    c = [0.0] * (n * n)
    for i in range(n):
        row = i * n
        for j in range(n):
            s = 0.0
            for k in range(n):
                s += a[row + k] * b[k * n + j]
            c[row + j] = s
    return c


def matmul_checksum(c) -> str:
    total = 0
    for v in c:
        total += int(v)
    return f"{total}:{h31(c)}"


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
