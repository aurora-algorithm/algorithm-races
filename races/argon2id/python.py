# Race: argon2id (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

from pathlib import Path


ARGON2_M, ARGON2_T, ARGON2_P = 19456, 2, 1


ARGON2_REF = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk"


REF_PASSWORD = "correct horse battery staple"


def passwords(n: int, _data: Path) -> list[str]:
    return [f"user-{i}-password" for i in range(n)]


def argon2_hasher():
    from argon2 import PasswordHasher, Type

    return PasswordHasher(
        time_cost=ARGON2_T, memory_cost=ARGON2_M, parallelism=ARGON2_P, hash_len=32, salt_len=16, type=Type.ID
    )


def argon2_hash(pws: list[str]) -> list[str]:
    hasher = argon2_hasher()
    return [hasher.hash(pw) for pw in pws]


def argon2_checksum(hashes: list[str]) -> str:
    from argon2.exceptions import VerifyMismatchError

    hasher = argon2_hasher()
    prefix = f"$argon2id$v=19$m={ARGON2_M},t={ARGON2_T},p={ARGON2_P}$"

    def verifies(h: str, pw: str) -> bool:
        try:
            return hasher.verify(h, pw)
        except VerifyMismatchError:
            return False

    ok = sum(h.startswith(prefix) and verifies(h, f"user-{i}-password") for i, h in enumerate(hashes))
    return f"{len(hashes)}:{ok}:{int(verifies(ARGON2_REF, REF_PASSWORD))}"
