# Race: bcrypt (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

from pathlib import Path


BCRYPT_COST = 10


BCRYPT_REF = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"


REF_PASSWORD = "correct horse battery staple"


def passwords(n: int, _data: Path) -> list[str]:
    return [f"user-{i}-password" for i in range(n)]


def bcrypt_hash(pws: list[str]) -> list[str]:
    import bcrypt

    return [bcrypt.hashpw(pw.encode(), bcrypt.gensalt(BCRYPT_COST)).decode() for pw in pws]


def bcrypt_checksum(hashes: list[str]) -> str:
    import bcrypt

    ok = sum(
        h[3:7] == f"${BCRYPT_COST}$" and bcrypt.checkpw(f"user-{i}-password".encode(), h.encode())
        for i, h in enumerate(hashes)
    )
    ref = bcrypt.checkpw(REF_PASSWORD.encode(), BCRYPT_REF.encode())
    return f"{len(hashes)}:{ok}:{int(ref)}"
