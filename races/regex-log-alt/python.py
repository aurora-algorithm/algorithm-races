# Race: regex-log-alt (python)
# Extracted from suite/python/misc.py — build and run via suite/run.py

import re


LOG_PATTERN = re.compile(r"(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])")


def regex_log(text: str) -> str:
    count = 0
    ids = 0
    for m in LOG_PATTERN.finditer(text):
        count += 1
        ids += int(m.group(2))
    return f"{count}:{ids}"
