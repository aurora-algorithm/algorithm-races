"""main.py under CPython's experimental JIT (kernels/SPEC.md section 14): run with PYTHON_JIT=1.

The python.org 3.14 builds ship the copy-and-patch JIT switched off; PYTHON_JIT=1 at startup turns it on. This file
only checks that it really is on (sys._jit, new in 3.14) and says so in the runtime field, so a build without the JIT
or a missing variable fails instead of passing as a JIT result.
"""

import sys

import main as driver


def main() -> int:
    jit = getattr(sys, "_jit", None)
    if jit is None or not jit.is_enabled():
        available = jit is not None and jit.is_available()
        print(f"the JIT is off (available in this build: {available}); run with PYTHON_JIT=1", file=sys.stderr)
        return 3
    plain = driver.runtime_info
    driver.runtime_info = lambda use_numpy, kernel: f"{plain(use_numpy, kernel)} jit=on"
    return driver.main()


if __name__ == "__main__":
    sys.exit(main())
