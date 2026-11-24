# Race: gunzip (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

import gzip
from pathlib import Path
from gen import fnv1a


def corpus(n: int, data_dir: Path) -> bytes:
    return (data_dir / f"corpus-{n}.bin").read_bytes()


def gzip_compress(raw: bytes) -> bytes:
    return gzip.compress(raw, compresslevel=6, mtime=0)


def gunzip(packed: bytes) -> bytes:
    return gzip.decompress(packed)


def plain_checksum(raw: bytes) -> str:
    return f"{len(raw)}:{fnv1a(raw)}"
