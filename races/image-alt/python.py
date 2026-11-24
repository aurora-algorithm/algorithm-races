# Race: image-alt (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

from pathlib import Path


PHOTOS = 8


TARGET_WIDTH = 1080


JPEG_QUALITY = 80


def photos(n: int, data: Path) -> list[bytes]:
    return [(data / "photos" / f"photo-{i % PHOTOS}.jpg").read_bytes() for i in range(n)]


def image_pyvips(images: list[bytes]) -> tuple[int, int, int, int]:
    import pyvips

    # libvips caches operations by their arguments; a warm repeat on the same bytes would be a cache hit.
    pyvips.cache_set_max(0)
    out_bytes = 0
    for data in images:
        small = pyvips.Image.thumbnail_buffer(data, TARGET_WIDTH)
        out_bytes += len(small.jpegsave_buffer(Q=JPEG_QUALITY))
    return len(images), small.width, small.height, out_bytes


def image_checksum(result: tuple[int, int, int, int]) -> tuple[str, dict]:
    count, width, height, out_bytes = result
    return f"{count}:{width}x{height}", {"out_bytes": out_bytes}
