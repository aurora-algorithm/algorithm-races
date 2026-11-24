"""Input files in kernels/data: orders-<N>.json for `json` and the files of the "work" kernels (kernels.json "data").
Written once by the Python generators; the photos need Pillow, so they run under the py-work venv."""

import os
import shutil
import subprocess
import sys
from pathlib import Path

from harness.config import IS_WIN, ROOT, resolve

DATA = ROOT / "data"
PY = ROOT / "python"
PHOTOS = 8
WORK_PY = "{env:PYWORK|" + ("../.venv-work/Scripts/python.exe" if IS_WIN else "../.venv-work/bin/python") + "}"


def _orders_json(size: int) -> None:
    path = DATA / f"orders-{size}.json"
    if path.exists():
        return
    sys.path.insert(0, str(PY))
    from gen import orders_json

    tmp = path.with_suffix(".tmp")
    tmp.write_text(orders_json(size), encoding="utf-8")
    tmp.replace(path)


def _work_file(kind: str, size: int) -> None:
    sys.path.insert(0, str(PY))
    import gen_work

    gen_work.write(kind, size, DATA)


def _photos() -> None:
    if all((DATA / "photos" / f"photo-{i}.jpg").exists() for i in range(PHOTOS)):
        return
    exe = Path(resolve(WORK_PY))
    exe = exe if exe.is_absolute() else Path(os.path.normpath(PY / exe))
    if not exe.exists():
        raise RuntimeError(f"{exe} missing: the photos need Pillow; run `run.py build --runtime py-work` first")
    print(f"generating {PHOTOS} photos in {DATA / 'photos'} (about 3 s each)", flush=True)
    subprocess.run([str(exe), "photos.py", str(PHOTOS), str(DATA)], cwd=PY, check=True)


def _words() -> None:
    path = DATA / "words-web2.txt"
    if not path.exists():
        shutil.copyfile(ROOT / "inputs" / "words-web2.txt", path)


def ensure(name: str, kernel: dict, size: int) -> None:
    DATA.mkdir(exist_ok=True)
    kind = "orders" if name == "json" else kernel.get("data")
    if kind == "words":
        _words()
    elif kind == "orders":
        _orders_json(size)
    elif kind == "photos":
        _photos()
    elif kind:
        _work_file(kind, size)
