"""The builtins group's many-words jobs (kernels/builtins-NOTES.md, "Many words"): find every place any of K words
occurs in a log, the ways a Python programmer writes it. Kernel names are bwords-K.python.WAY.

Every way gets the same log and the same K words and prints `"{K}:{matches}:{sum of match start offsets}"`. The words
are 8-digit user ids, and each line hides its user id inside a longer run of digits (a session token), so splitting
the line into tokens cannot find them. No two matches overlap in these logs (checked at every size), so every way
counts the same matches. The bwords-tok-K jobs use the older log, where the id is a whole path segment, to show the
split-and-set way that only works there.
"""

import re

from gen import METHODS, STATUSES, XorShift32

LANG = "python"
POOL = 10_000
WORD_COUNTS = (1, 10, 100, 1000)


def digits(rng: XorShift32) -> str:
    """0 to 6 random digits: the length first, then each digit."""
    return "".join(str(rng.next() % 10) for _ in range(rng.next() % 7))


def words_log(lines: int, tokens: bool = False) -> tuple[str, list[str]]:
    """The log and the user id of each line, from one generator: first the id pool, then each line's fields.

    By default the id sits inside the session token `sid=<digits><id><digits>`; with tokens=True it is the whole last
    path segment `/users/<id>` (the bwords-tok jobs)."""
    rng = XorShift32()
    pool = [str(10_000_000 + rng.next() % 90_000_000) for _ in range(POOL)]
    parts = []
    users = []
    for _ in range(lines):
        method = METHODS[rng.next() % 4]
        version = 1 + rng.next() % 3
        user = pool[rng.next() % POOL]
        users.append(user)
        if tokens:
            where = f"users/{user}"
        else:
            before = digits(rng)
            where = f"cart?sid={before}{user}{digits(rng)}"
        status = STATUSES[rng.next() % 12]
        ms = rng.next() % 1000
        parts.append(f"{method} /api/v{version}/{where} {status} {ms}ms\n")
    return "".join(parts), users


def first_distinct(users: list[str], k: int) -> list[str]:
    """The words: the ids of the log's lines in line order, repeats skipped, the first k."""
    seen: dict[str, None] = {}
    for w in users:
        seen.setdefault(w, None)
        if len(seen) == k:
            break
    return list(seen)


def find_loop(text: str, words: list[str]) -> tuple[int, int]:
    matches = offsets = 0
    for w in words:
        i = text.find(w)
        while i != -1:
            matches += 1
            offsets += i
            i = text.find(w, i + 1)
    return matches, offsets


def regex_alternation(text: str, words: list[str]) -> tuple[int, int]:
    pattern = re.compile("|".join(words))
    matches = offsets = 0
    for m in pattern.finditer(text):
        matches += 1
        offsets += m.start()
    return matches, offsets


def pyahocorasick(text: str, words: list[str]) -> tuple[int, int]:
    import ahocorasick

    automaton = ahocorasick.Automaton()
    for w in words:
        automaton.add_word(w, len(w))
    automaton.make_automaton()
    matches = offsets = 0
    for end, length in automaton.iter(text):
        matches += 1
        offsets += end - length + 1
    return matches, offsets


def split_set(text: str, words: list[str]) -> tuple[int, int]:
    """Only for the whole-token log: take each line's last path segment and look it up in a set."""
    wanted = set(words)
    matches = offsets = 0
    pos = 0
    for line in text.splitlines(keepends=True):
        method, path, _ = line.split(" ", 2)
        cut = path.rfind("/") + 1
        if path[cut:] in wanted:
            matches += 1
            offsets += pos + len(method) + 1 + cut
        pos += len(line)
    return matches, offsets


WAYS = {"str.find": find_loop, "re": regex_alternation, "pyahocorasick": pyahocorasick}
TOKEN_WAYS = {"split-set": split_set, "pyahocorasick": pyahocorasick}


def entry(k: int, run, tokens: bool = False) -> dict:
    def prepare(lines: int, _data):
        text, users = words_log(lines, tokens)
        return text, first_distinct(users, k)

    return {
        "prepare": prepare,
        "run": lambda x, _t, _p: run(*x),
        "check": lambda r, k=k: f"{k}:{r[0]}:{r[1]}",
    }


def kernels() -> dict:
    embedded = {f"bwords-{k}.{LANG}.{way}": entry(k, fn) for k in WORD_COUNTS for way, fn in WAYS.items()}
    whole = {
        f"bwords-tok-{k}.{LANG}.{way}": entry(k, fn, tokens=True) for k in WORD_COUNTS for way, fn in TOKEN_WAYS.items()
    }
    return embedded | whole
