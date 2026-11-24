# Race: wordcount (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

from pathlib import Path


def text_path(n: int, data: Path) -> Path:
    return data / f"text-{n}.txt"


def wordcount(path: Path) -> dict[str, int]:
    counts: dict[str, int] = {}
    with path.open(encoding="utf-8") as f:
        for line in f:
            for word in line.split():
                counts[word] = counts.get(word, 0) + 1
    return counts


def wordcount_checksum(counts: dict[str, int]) -> str:
    top = min(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    return f"{sum(counts.values())}:{len(counts)}:{top[0]}:{top[1]}"
