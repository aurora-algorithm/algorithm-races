# Race: aho-corasick (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

DNA = b"ACGT"


def aho_corasick(data: tuple[list[bytes], bytes]) -> str:
    patterns, text = data
    code = [0] * 256
    for i, ch in enumerate(DNA):
        code[ch] = i
    goto = [-1] * 4
    out = [0]
    for pattern in patterns:
        s = 0
        for ch in pattern:
            edge = s * 4 + code[ch]
            if goto[edge] == -1:
                goto[edge] = len(out)
                goto.extend((-1, -1, -1, -1))
                out.append(0)
            s = goto[edge]
        out[s] += 1
    fail = [0] * len(out)
    queue = []
    for c in range(4):
        if goto[c] == -1:
            goto[c] = 0
        else:
            queue.append(goto[c])
    head = 0
    while head < len(queue):
        s = queue[head]
        head += 1
        out[s] += out[fail[s]]
        for c in range(4):
            t = goto[s * 4 + c]
            if t == -1:
                goto[s * 4 + c] = goto[fail[s] * 4 + c]
            else:
                fail[t] = goto[fail[s] * 4 + c]
                queue.append(t)
    s = matches = hits = 0
    for ch in text:
        s = goto[s * 4 + code[ch]]
        found = out[s]
        if found:
            matches += found
            hits += 1
    return f"{len(patterns)}:{len(out)}:{matches}:{hits}"
