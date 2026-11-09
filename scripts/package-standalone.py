#!/usr/bin/env python3
"""Build a KUAL USB install ZIP from a verified Kindle PW1 ARM executable."""
import argparse
from pathlib import Path
import struct
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parent.parent
NAME = "Kindle-Golden-Hero-KPW1-Standalone-v0.1.0.zip"


def validate_binary(binary):
    data = binary.read_bytes()
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("Expected a 32-bit little-endian ELF executable")
    elf_type, machine, _, _, program_offset, _, flags, _, entry_size, count = (
        struct.unpack_from("<HHIIIIIHHH", data, 16)
    )
    if elf_type not in (2, 3) or machine != 40 or flags >> 24 != 5 or flags & 0x400:
        raise ValueError("Expected ARM EABI with soft-float calling conventions")
    if entry_size != 32 or program_offset + count * entry_size > len(data):
        raise ValueError("Invalid ELF program table")
    for i in range(count):
        entry = struct.unpack_from("<8I", data, program_offset + i * entry_size)
        if entry[0] == 3:
            offset, size = entry[1], entry[4]
            if data[offset:offset + size] == b"/lib/ld-linux.so.3\0":
                return data
    raise ValueError("Expected Kindle PW1 ELF interpreter /lib/ld-linux.so.3")


def pack(binary, output):
    executable = validate_binary(binary)
    extension = "extensions/GoldenHeroStandalone"
    source = ROOT / "kual" / extension
    files = [
        ("golden_hero_standalone/golden_hero_standalone", executable, 0o755),
        ("golden_hero_standalone/README.md", (ROOT / "docs/STANDALONE.md").read_bytes(), 0o644),
        ("golden_hero_standalone/LICENSE", (ROOT / "LICENSE").read_bytes(), 0o644),
        ("golden_hero_standalone/FONT-OFL.txt", (ROOT / "game/FONT-OFL.txt").read_bytes(), 0o644),
        (f"{extension}/config.xml", (source / "config.xml").read_bytes(), 0o644),
        (f"{extension}/menu.json", (source / "menu.json").read_bytes(), 0o644),
        (f"{extension}/launch.sh", (source / "launch.sh").read_bytes(), 0o755),
        (f"{extension}/guarded-test.sh",
         (source.parent / "GoldenHeroFullscreen/guarded-test.sh").read_bytes(), 0o755),
    ]
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
        for name, content, mode in files:
            info = ZipInfo(name)
            info.create_system = 3
            info.external_attr = (0o100000 | mode) << 16
            if name.endswith(".sh"):
                content = content.replace(b"\r\n", b"\n")
            archive.writestr(info, content, compress_type=ZIP_DEFLATED)
    print(f"Created {output} ({output.stat().st_size:,} bytes)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path,
                        default=ROOT / "build-standalone-armel/golden_hero_standalone")
    parser.add_argument("--output", type=Path, default=ROOT / NAME)
    args = parser.parse_args()
    try:
        pack(args.binary, args.output)
    except (OSError, ValueError) as error:
        parser.exit(2, f"{error}\n")


if __name__ == "__main__":
    main()
