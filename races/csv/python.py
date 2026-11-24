# Race: csv (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

from pathlib import Path


def csv_path(n: int, data: Path) -> Path:
    return data / f"orders-{n}.csv"


def csv_stdlib(path: Path) -> tuple[int, dict[int, int]]:
    import csv

    totals: dict[int, int] = {}
    rows = 0
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        cust, total = header.index("customer_id"), header.index("total_cents")
        for row in reader:
            rows += 1
            key = int(row[cust])
            totals[key] = totals.get(key, 0) + int(row[total])
    return rows, totals


def csv_checksum(result: tuple[int, dict[int, int]]) -> str:
    rows, totals = result
    top = min(totals.items(), key=lambda kv: (-kv[1], kv[0]))
    return f"{rows}:{len(totals)}:{sum(totals.values())}:{top[0]}:{top[1]}"
