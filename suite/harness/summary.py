"""kernels.jsonl -> summary.csv: one row per (kernel, threads, runtime), medians over rounds, checksum agreement."""

import csv
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path

COLUMNS = [
    "kernel",
    "group",
    "threads",
    "size",
    "runtime",
    "label",
    "median_ms",
    "first_ms",
    "min_ms",
    "x_fastest",
    "process_ms",
    "cpu_ms",
    "peak_rss_mb",
    "repeats",
    "rounds",
    "checksum",
    "checksum_ok",
    "note",
]


def load(jsonl: Path) -> list[dict]:
    return [json.loads(line) for line in jsonl.read_text(encoding="utf-8").splitlines() if line.strip()]


def _med(values) -> float | None:
    values = [v for v in values if v is not None]
    return round(statistics.median(values), 3) if values else None


def consensus(rows: list[dict]) -> dict:
    """Most common checksum per (kernel, size), over every runtime and thread count."""
    votes = defaultdict(Counter)
    for r in rows:
        if r.get("checksum"):
            votes[(r["kernel"], r["size"])][r["checksum"]] += 1
    return {k: c.most_common(1)[0][0] for k, c in votes.items()}


def summarize(rows: list[dict]) -> list[dict]:
    agree = consensus(rows)
    cells = defaultdict(list)
    for r in rows:
        cells[(r["kernel"], r["threads"], r["runtime"])].append(r)
    out = []
    for (kernel, threads, runtime), rs in cells.items():
        good = [r for r in rs if "error" not in r.get("warm", {"error": 1})]
        first = rs[0]
        sums = {r.get("checksum") for r in rs}
        majority = agree.get((kernel, first["size"]))
        want = first.get("expected") or majority
        ok = bool(good) and len(sums) == 1 and first.get("checksum") == want == majority
        notes = [r["cold"].get("error") or r.get("warm", {}).get("error") for r in rs]
        out.append(
            {
                "kernel": kernel,
                "group": first["group"],
                "threads": threads,
                "size": first["size"],
                "runtime": runtime,
                "label": first["label"],
                "median_ms": _med(r["warm"]["median_ms"] for r in good),
                "first_ms": _med(r["warm"]["first_ms"] for r in good),
                "min_ms": min((r["warm"]["min_ms"] for r in good), default=None),
                "process_ms": _med(r["cold"]["wall_ms"] for r in rs),
                "cpu_ms": _med(r["cold"].get("cpu_ms") for r in rs),
                "peak_rss_mb": _med(r["cold"].get("peak_rss_mb") for r in rs),
                "repeats": good[0]["warm"]["repeats"] if good else 0,
                "rounds": len(rs),
                "checksum": first.get("checksum"),
                "checksum_ok": ok,
                "note": "; ".join(sorted({n for n in notes if n})),
            }
        )
    fastest = defaultdict(lambda: float("inf"))
    for r in out:
        # A 0 ms median (a quick-size kernel under the timer's resolution) can't be a divisor.
        if r["median_ms"] and r["checksum_ok"]:
            key = (r["kernel"], r["threads"])
            fastest[key] = min(fastest[key], r["median_ms"])
    for r in out:
        best = fastest[(r["kernel"], r["threads"])]
        r["x_fastest"] = round(r["median_ms"] / best, 2) if r["median_ms"] and best != float("inf") else None
    return sorted(out, key=lambda r: (r["kernel"], r["threads"], r["median_ms"] or float("inf")))


def write(rows: list[dict], path: Path) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=COLUMNS)
        w.writeheader()
        w.writerows(rows)


def table(rows: list[dict]) -> str:
    lines = [
        f"{'kernel':16} {'T':>2} {'runtime':12} {'median ms':>11} {'first ms':>10} {'process ms':>10} "
        f"{'RSS MB':>8} {'x':>6}  checksum"
    ]
    for r in rows:
        lines.append(
            f"{r['kernel']:16} {r['threads']:>2} {r['runtime']:12} {_f(r['median_ms']):>11} {_f(r['first_ms']):>10} "
            f"{_f(r['process_ms']):>10} {_f(r['peak_rss_mb']):>8} {_f(r['x_fastest']):>6}  "
            f"{'ok' if r['checksum_ok'] else 'MISMATCH ' + str(r['checksum'])} {r['note']}"
        )
    return "\n".join(lines)


def _f(x) -> str:
    return "-" if x is None else f"{x:,.1f}" if isinstance(x, float) else str(x)
