#!/usr/bin/env python3
"""Regenerate the source-controlled 8 KiB blank CHR-ROM bank."""

from pathlib import Path


def main() -> None:
    output = Path(__file__).resolve().parents[1] / "assets" / "blank.chr"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(bytes(8192))
    print(f"wrote {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
