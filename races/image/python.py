# Race: image (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

import io
from pathlib import Path


PHOTOS = 8


TARGET_WIDTH = 1080


JPEG_QUALITY = 80


def photos(n: int, data: Path) -> list[bytes]:
    return [(data / "photos" / f"photo-{i % PHOTOS}.jpg").read_bytes() for i in range(n)]


def target_height(width: int, height: int) -> int:
    return (height * TARGET_WIDTH + width // 2) // width


def image_pillow(images: list[bytes]) -> tuple[int, int, int, int]:
    from PIL import Image

    out_bytes = 0
    for data in images:
        with Image.open(io.BytesIO(data)) as img:
            small = img.resize((TARGET_WIDTH, target_height(*img.size)), Image.Resampling.LANCZOS)
        buf = io.BytesIO()
        small.save(buf, "JPEG", quality=JPEG_QUALITY)
        out_bytes += buf.tell()
    return len(images), small.width, small.height, out_bytes


def image_checksum(result: tuple[int, int, int, int]) -> tuple[str, dict]:
    count, width, height, out_bytes = result
    return f"{count}:{width}x{height}", {"out_bytes": out_bytes}
