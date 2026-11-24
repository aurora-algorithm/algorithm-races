# Race: huffman (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

import heapq


def huffman(text: bytes) -> tuple[int, int, bytes, bool]:
    freq = [0] * 256
    for b in text:
        freq[b] += 1
    heap = [f * 1024 + sym for sym, f in enumerate(freq) if f]
    heapq.heapify(heap)
    left = [0] * 512
    right = [0] * 512
    next_id = 256
    while len(heap) > 1:
        a = heapq.heappop(heap)
        b = heapq.heappop(heap)
        left[next_id], right[next_id] = a % 1024, b % 1024
        heapq.heappush(heap, (a // 1024 + b // 1024) * 1024 + next_id)
        next_id += 1
    root = heap[0] % 1024
    codes = [0] * 256
    lengths = [0] * 256
    stack = [(root, 0, 0)]
    while stack:
        node, code, length = stack.pop()
        if node < 256:
            codes[node], lengths[node] = code, length
        else:
            stack.append((left[node], code << 1, length + 1))
            stack.append((right[node], (code << 1) | 1, length + 1))
    encoded = bytearray()
    acc = nbits = bits = 0
    for b in text:
        acc = (acc << lengths[b]) | codes[b]
        nbits += lengths[b]
        bits += lengths[b]
        while nbits >= 8:
            nbits -= 8
            encoded.append((acc >> nbits) & 0xFF)
        acc &= (1 << nbits) - 1
    if nbits:
        encoded.append((acc << (8 - nbits)) & 0xFF)
    decoded = bytearray(len(text))
    node = root
    k = 0
    for byte in encoded:
        for shift in range(7, -1, -1):
            node = right[node] if (byte >> shift) & 1 else left[node]
            if node < 256:
                decoded[k] = node
                k += 1
                node = root
                if k == len(text):
                    break
    return len(text), bits, bytes(encoded), decoded == text
