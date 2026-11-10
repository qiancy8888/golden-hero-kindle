#!/usr/bin/env python3
"""Package the PW1 KUAL runtime and its corresponding source without the game ROM."""
import argparse
import hashlib
import struct
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


ROOT = Path(__file__).resolve().parents[1]
GAMBATTE_SHA256 = "09896e086b546b1c8f284bc6767039d362eaff8bad267796fdf10137f4a0173f"


def verify_arm(data):
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("Expected 32-bit little-endian ELF")
    elf_type, machine, _, _, offset, _, flags, _, size, count = struct.unpack_from("<HHIIIIIHHH", data, 16)
    if elf_type != 2 or machine != 40 or flags >> 24 != 5 or flags & 0x400:
        raise ValueError("Expected ARM EABI5 executable with soft-float calling conventions")
    if offset < 52 or size != 32 or offset + count * size > len(data):
        raise ValueError("Invalid ELF program table")
    segments = {struct.unpack_from("<I", data, offset + i * size)[0] for i in range(count)}
    if 1 not in segments or segments.intersection({2, 3}):
        raise ValueError("Expected a static executable with a loadable segment")


def add(archive, name, data, mode=0o644):
    entry = ZipInfo(name)
    entry.create_system = 3
    entry.external_attr = (0o100000 | mode) << 16
    if name.endswith(".sh"):
        data = data.replace(b"\r\n", b"\n")
    archive.writestr(entry, data, compress_type=ZIP_DEFLATED)


def pack(binary, gambatte_archive, zig_dir, output):
    executable = binary.read_bytes()
    verify_arm(executable)
    gambatte = gambatte_archive.read_bytes()
    if hashlib.sha256(gambatte).hexdigest() != GAMBATTE_SHA256:
        raise ValueError("Gambatte source must be the pinned codeload ZIP in docs/PLATINUM.md")
    extension = "extensions/PlatinumHero"
    guard = "kual/extensions/GoldenHeroFullscreen/guarded-test.sh"
    sources = [
        "CMakeLists.txt", "cmake/Platinum.cmake", "LICENSE", "docs/PLATINUM.md",
        "kindle/platinum.cpp", "kindle/platinum_core.cpp", "kindle/platinum_core.h",
        "kindle/fb.cpp", "kindle/fb.h", "kindle/touch.cpp", "kindle/touch.h",
        "scripts/package-platinum.py", "scripts/build-platinum-rom.py", "scripts/gmud-menu-shift.c",
        "tests/test_platinum.cpp", "tests/test_platinum_guard.py", guard,
    ]
    toolchain_licenses = {
        "zig": "LICENSE",
        "musl": "lib/libc/musl/COPYRIGHT",
        "libcxx": "lib/libcxx/LICENSE.TXT",
        "libcxxabi": "lib/libcxxabi/LICENSE.TXT",
        "libunwind": "lib/libunwind/LICENSE.TXT",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(gambatte_archive) as upstream, ZipFile(output, "w") as archive:
        copy = next(name for name in upstream.namelist() if name.count("/") == 1 and name.endswith("/COPYING"))
        add(archive, "platinum_hero/platinum_hero", executable, 0o755)
        add(archive, "platinum_hero/README.md", (ROOT / "docs/PLATINUM.md").read_bytes())
        add(archive, "platinum_hero/licenses/Gambatte.txt", upstream.read(copy))
        for license_name, path in toolchain_licenses.items():
            add(archive, f"platinum_hero/licenses/{license_name}.txt", (zig_dir / path).read_bytes())
        add(archive, "platinum_hero/source/gambatte.zip", gambatte)
        for name in ("config.xml", "menu.json", "launch.sh"):
            path = f"kual/{extension}/{name}"
            content = (ROOT / path).read_bytes()
            add(archive, f"{extension}/{name}", content, 0o755 if name.endswith(".sh") else 0o644)
            add(archive, f"platinum_hero/source/{path}", content)
        add(archive, f"{extension}/guarded-test.sh", (ROOT / guard).read_bytes(), 0o755)
        for path in sources:
            add(archive, f"platinum_hero/source/{path}", (ROOT / path).read_bytes())
    print(f"Created {output} ({output.stat().st_size:,} bytes)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--gambatte-archive", required=True, type=Path)
    parser.add_argument("--zig-dir", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "Kindle-Platinum-Hero-PW1-v1.0.0.zip")
    args = parser.parse_args()
    try:
        pack(args.binary, args.gambatte_archive, args.zig_dir, args.output)
    except (OSError, ValueError) as error:
        parser.exit(2, f"{error}\n")


if __name__ == "__main__":
    main()
