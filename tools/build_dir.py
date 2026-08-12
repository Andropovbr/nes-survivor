#!/usr/bin/env python3
"""Create or remove the repository build directory on any host platform."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = (PROJECT_ROOT / "build").resolve()


def main() -> int:
    if len(sys.argv) != 2 or sys.argv[1] not in {"create", "clean"}:
        print("usage: build_dir.py {create|clean}", file=sys.stderr)
        return 2

    if BUILD_DIR.parent != PROJECT_ROOT:
        print("refusing to operate outside the project root", file=sys.stderr)
        return 1

    if sys.argv[1] == "create":
        BUILD_DIR.mkdir(parents=True, exist_ok=True)
    elif BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
