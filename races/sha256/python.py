# Race: sha256 (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

import hashlib


def sha256(buf: bytes) -> str:
    return hashlib.sha256(buf).hexdigest()
