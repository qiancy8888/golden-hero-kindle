#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scripts.fetch_upstream import patch_nc1020_source


def check(linebreak: bytes) -> None:
    source = (
        b"\t\trom_volume1[i] = rom_buff + (0x8000 * (0x100 + i));"
        + linebreak
        + b"\t\trom_volume1[i] = rom_buff + (0x8000 * (0x200 + i));"
    )
    fixed = patch_nc1020_source(source)
    assert b"\t\trom_volume1[i] = rom_buff + (0x8000 * (0x100 + i));" in fixed
    assert b"\t\trom_volume2[i] = rom_buff + (0x8000 * (0x200 + i));" in fixed
    assert b"\t\trom_volume1[i] = rom_buff + (0x8000 * (0x200 + i));" not in fixed


if __name__ == "__main__":
    check(b"\n")
    check(b"\r\n")
    print("NC1020 ROM volume 2 patch tests passed.")
