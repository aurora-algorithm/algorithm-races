"""The 24 MP test photos of the `image` kernels (kernels/SPEC.md section 11): synthetic, generated here, so there is
no licence to track. Needs Pillow (the py-work venv); run.py calls it through that venv.

Each photo is 6000 x 4000: per channel a fractal noise field (random grids from xorshift32, upscaled bicubic and
summed, coarse to fine), a sky gradient, hard-edged shapes (edges for the resampler to work on) and a tiled sensor
grain, saved as JPEG quality 90, 4:2:0, like a camera's output.

usage: python photos.py COUNT DIR
"""

import hashlib
import sys
from array import array
from pathlib import Path

from gen import SEED, XorShift32
from PIL import Image, ImageChops, ImageDraw, ImageFilter

WIDTH, HEIGHT = 6000, 4000
OCTAVES = 9
GRAIN = 512


def noise(rng: XorShift32, w: int, h: int) -> Image.Image:
    words = (w * h + 3) // 4
    return Image.frombytes("L", (w, h), array("I", rng.ints(words)).tobytes()[: w * h])


def fractal(rng: XorShift32) -> Image.Image:
    field = Image.new("L", (WIDTH, HEIGHT), 128)
    for octave in range(OCTAVES):
        w, h = 6 << octave, 4 << octave
        layer = noise(rng, w, h).resize((WIDTH, HEIGHT), Image.Resampling.BICUBIC)
        field = Image.blend(field, layer, 0.55 if octave == 0 else 0.5 / (1 + octave * 0.35))
    return field


def shapes(rng: XorShift32, img: Image.Image) -> None:
    draw = ImageDraw.Draw(img)
    for _ in range(40):
        x, y = rng.next() % WIDTH, HEIGHT // 3 + rng.next() % (HEIGHT * 2 // 3)
        w, h = 80 + rng.next() % 900, 80 + rng.next() % 700
        colour = (rng.next() % 256, rng.next() % 256, rng.next() % 256)
        if rng.next() % 2:
            draw.rectangle((x, y, x + w, y + h), fill=colour)
        else:
            draw.ellipse((x, y, x + w, y + h), fill=colour)


def photo(index: int) -> Image.Image:
    rng = XorShift32(SEED + 7919 * index)
    rgb = Image.merge("RGB", [fractal(rng) for _ in range(3)])
    sky = Image.linear_gradient("L").resize((WIDTH, HEIGHT)).convert("RGB")
    rgb = Image.blend(rgb, ImageChops.multiply(sky, Image.new("RGB", sky.size, (150, 190, 255))), 0.3)
    shapes(rng, rgb)
    rgb = rgb.filter(ImageFilter.GaussianBlur(1.2))
    tile = Image.merge("RGB", [noise(rng, GRAIN, GRAIN) for _ in range(3)])
    grain = Image.new("RGB", (WIDTH, HEIGHT))
    for y in range(0, HEIGHT, GRAIN):
        for x in range(0, WIDTH, GRAIN):
            grain.paste(tile, (x, y))
    return Image.blend(rgb, grain, 0.12)


def write(count: int, data: Path) -> Path:
    out = data / "photos"
    out.mkdir(parents=True, exist_ok=True)
    for i in range(count):
        path = out / f"photo-{i}.jpg"
        if not path.exists():
            tmp = path.with_name(path.name + ".tmp")
            photo(i).save(tmp, "JPEG", quality=90, subsampling="4:2:0")
            tmp.replace(path)
    sums = [f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in sorted(out.glob("photo-*.jpg"))]
    (out / "SHA256SUMS").write_text("".join(sums), encoding="ascii")
    return out


if __name__ == "__main__":
    print(write(int(sys.argv[1]), Path(sys.argv[2])))
