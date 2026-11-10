#!/usr/bin/env python3
"""Build the pinned GMUD_GB source privately with GBDK 2020 4.5.0."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def fix_npc_bank(source_dir):
    # npc.c calls bank 25 from bank 26. GBDK needs BANKED at both ends of that call.
    for filename in ("text.h", "textui.c"):
        path = source_dir / filename
        source = path.read_text(encoding="utf-8")
        signature = "void text_load_npc(uint8_t id)"
        if source.count(signature) != 1:
            raise ValueError(f"Unexpected NPC loader in {filename}; use the pinned GMUD_GB source")
        if signature + " BANKED" not in source:
            path.write_text(source.replace(signature, signature + " BANKED"), encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gmud", required=True, type=Path, help="path to the GMUD_GB source tree")
    parser.add_argument("--lcc", required=True, type=Path, help="path to the GBDK lcc or lcc.exe")
    args = parser.parse_args()
    root = args.gmud.resolve()
    lcc = args.lcc.resolve()
    if not lcc.is_file() or not (root / "src/textui.c").is_file() or not (root / "tools/check_home.py").is_file():
        parser.error("Expected GBDK lcc and the GMUD_GB source tree documented in docs/PLATINUM.md")

    build = root / "build"
    build.mkdir(exist_ok=True)
    flags = ["-DMF_SHOW_OWNS_INPUT=0x04", "-Wm-yC", "-Wm-yt0x1B", "-Wm-ya4", "-Wm-yo64",
             "-Wm-ynHEROTAN", "-Wl-m", "-Wl-j"]
    with tempfile.TemporaryDirectory(prefix="pw1-", dir=build) as directory:
        stage = Path(directory)
        source_dir = stage / "src"
        shutil.copytree(root / "src", source_dir)
        fix_npc_bank(source_dir)
        shutil.copyfile(Path(__file__).with_name("gmud-menu-shift.c"), source_dir / "pw1_menu_shift.c")
        sources = sorted(source_dir.glob("*.c"))
        subprocess.run([lcc, *flags, "-o", (stage / "hero.gbc").relative_to(root).as_posix(),
                        *(path.relative_to(root).as_posix() for path in sources)], cwd=root, check=True)
        subprocess.run([sys.executable, "-X", "utf8", "tools/check_home.py",
                        (stage / "hero.noi").relative_to(root).as_posix()], cwd=root, check=True)
        for name in ("hero.map", "hero.noi", "hero.gbc"):
            (stage / name).replace(build / name)
    print(build / "hero.gbc")


if __name__ == "__main__":
    main()
