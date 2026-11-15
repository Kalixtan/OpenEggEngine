#!/usr/bin/env python3
"""Convert a CHIP-8 ROM to the text format read by Main.gm."""

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="input CHIP-8 ROM")
    parser.add_argument("destination", type=Path, help="output hex text file")
    args = parser.parse_args()

    data = args.source.read_bytes()
    if len(data) > 0xE00:
        parser.error("classic CHIP-8 ROM must fit in 3,584 bytes")
    lines = [data[i:i + 32].hex() for i in range(0, len(data), 32)]
    args.destination.write_text("\n".join(lines) + "\n", encoding="ascii")


if __name__ == "__main__":
    main()
