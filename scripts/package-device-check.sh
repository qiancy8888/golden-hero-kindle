#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
OUT=Kindle-KPW1-DeviceCheck-v0.3.zip
STAGE=build-device-check-root
rm -rf "$STAGE"
mkdir -p "$STAGE/extensions/GoldenHeroCheck"
cp kual/device-check/config.xml "$STAGE/extensions/GoldenHeroCheck/"
cp kual/device-check/menu.json "$STAGE/extensions/GoldenHeroCheck/"
cp kual/device-check/diagnose.sh "$STAGE/extensions/GoldenHeroCheck/"
cp kual/device-check/README_安装说明.txt "$STAGE/extensions/GoldenHeroCheck/"
python3 - <<'PY'
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
root=Path('build-device-check-root')
out=Path('Kindle-KPW1-DeviceCheck-v0.3.zip')
with ZipFile(out,'w',ZIP_DEFLATED) as z:
    for p in root.rglob('*'):
        if p.is_file(): z.write(p,p.relative_to(root))
print(f'Created {out} ({out.stat().st_size:,} bytes).')
PY
