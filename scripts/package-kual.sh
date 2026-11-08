#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN=build-armel/golden_hero

[ -f "$BIN" ] || { echo "Missing KPW1 ARM binary; run scripts/build-armel.sh first." >&2; exit 2; }
[ -s roms/obj_lu.bin ] || { echo "Missing user-provided roms/obj_lu.bin (24 MiB). No game ROM is bundled." >&2; exit 3; }
[ -s roms/nc1020.fls ] || { echo "Missing user-provided roms/nc1020.fls (1 MiB). No game FLASH image is bundled." >&2; exit 4; }
[ "$(wc -c < roms/obj_lu.bin | tr -d ' ')" = 25165824 ] || { echo "obj_lu.bin has the wrong size." >&2; exit 5; }
[ "$(wc -c < roms/nc1020.fls | tr -d ' ')" = 1048576 ] || { echo "nc1020.fls has the wrong size." >&2; exit 6; }
command -v readelf >/dev/null 2>&1 || { echo "readelf is required." >&2; exit 7; }
readelf -h "$BIN" | grep -q 'Machine:.*ARM' || { echo "Refusing to package a non-ARM executable." >&2; exit 8; }
if readelf -A "$BIN" | grep -q 'Tag_ABI_VFP_args: VFP registers'; then
 echo "Refusing to package: hard-float ABI is not compatible with KPW1." >&2; exit 9
fi
readelf -l "$BIN" | grep -q '/lib/ld-linux.so.3' || {
 echo "Refusing to package: unexpected ELF interpreter; expected /lib/ld-linux.so.3." >&2; exit 10;
}
if readelf -d "$BIN" | grep -q 'libSDL'; then
 echo "Refusing to package: this port must not depend on SDL." >&2; exit 11
fi

OUT=release-root
rm -rf "$OUT"
mkdir -p "$OUT/golden_hero/roms" "$OUT/golden_hero/data" "$OUT/golden_hero/logs" "$OUT/extensions"
cp "$BIN" "$OUT/golden_hero/golden_hero"
cp roms/obj_lu.bin roms/nc1020.fls "$OUT/golden_hero/roms/"
cp roms/READ_ME.txt "$OUT/golden_hero/roms/"
cp data/READ_ME.txt "$OUT/golden_hero/data/"
cp README_中文.md "$OUT/golden_hero/"
cp -R kual/extensions/golden_hero "$OUT/extensions/"
python3 - <<'PY'
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
root=Path('release-root')
out=Path('Kindle-Golden-Hero-KPW1-install.zip')
with ZipFile(out,'w',ZIP_DEFLATED) as z:
    for p in root.rglob('*'):
        if p.is_file(): z.write(p,p.relative_to(root))
print(f'Created {out} ({out.stat().st_size:,} bytes).')
PY
