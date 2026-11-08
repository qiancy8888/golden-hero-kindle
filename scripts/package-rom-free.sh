#!/bin/sh
# Create a KUAL USB install ZIP containing the ARM binary but no ROMs.
set -eu
cd "$(dirname "$0")/.."
BIN=build-armel/golden_hero
OUT=Kindle-Golden-Hero-KPW1-v0.8-ROM-free.zip
STAGE=build-rom-free-root

[ -s "$BIN" ] || { echo "Missing ARM binary. Run scripts/build-armel.sh first." >&2; exit 2; }
command -v readelf >/dev/null 2>&1 || { echo "Missing readelf." >&2; exit 2; }
readelf -h "$BIN" | grep -q 'Machine:.*ARM' || { echo "Not an ARM executable." >&2; exit 3; }
if readelf -A "$BIN" | grep -q 'Tag_ABI_VFP_args: VFP registers'; then
 echo "Invalid ABI: ARM hard float" >&2; exit 3
fi
readelf -l "$BIN" | grep -q '/lib/ld-linux.so.3' || { echo "Incorrect ARM interpreter." >&2; exit 4; }
if readelf -d "$BIN" | grep -q 'libSDL'; then
 echo "Unexpected SDL dependency." >&2; exit 5
fi

rm -rf "$STAGE"
mkdir -p "$STAGE/golden_hero/roms" "$STAGE/golden_hero/data" "$STAGE/golden_hero/logs" "$STAGE/extensions" "$STAGE/golden_hero_diagnostics"
cp "$BIN" "$STAGE/golden_hero/golden_hero"
cp roms/READ_ME.txt "$STAGE/golden_hero/roms/"
cp data/READ_ME.txt "$STAGE/golden_hero/data/"
cp README_中文.md "$STAGE/golden_hero/"
cp -R kual/extensions/golden_hero "$STAGE/extensions/"
cp -R kual/extensions/GoldenHeroFullscreen "$STAGE/extensions/"
cp kual/golden_hero_diagnostics/raw-touch.lua "$STAGE/golden_hero_diagnostics/"

python3 - "$STAGE" "$OUT" <<'PY'
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import sys
root, output = map(Path, sys.argv[1:])
with ZipFile(output, 'w', ZIP_DEFLATED, compresslevel=9) as archive:
    for path in sorted(root.rglob('*')):
        if path.is_file():
            archive.write(path, path.relative_to(root))
with ZipFile(output) as archive:
    names = archive.namelist()
    assert 'golden_hero/golden_hero' in names
    assert 'extensions/golden_hero/menu.json' in names
    assert 'extensions/golden_hero/display-test.sh' in names
    assert 'extensions/GoldenHeroFullscreen/guarded-test.sh' in names
    assert 'extensions/GoldenHeroFullscreen/test-controls.sh' in names
    assert 'golden_hero_diagnostics/raw-touch.lua' in names
    assert not any(name.endswith(('obj_lu.bin','nc1020.fls', 'nc1020.sts')) for name in names)
    assert archive.getinfo('golden_hero/golden_hero').external_attr >> 16 & 0o111
    assert archive.getinfo('extensions/GoldenHeroFullscreen/test-controls.sh').external_attr >> 16 & 0o111
print(f'Created {output} ({output.stat().st_size:,} bytes) without game/firmware images.')
PY
